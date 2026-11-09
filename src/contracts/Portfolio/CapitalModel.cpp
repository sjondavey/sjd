#include <sjd/contracts/Portfolio/CapitalModel.h>

#include <sjd/tools/Errors.h>
#include <sjd/tools/Log.h>

#include <algorithm>
#include <list>
#include <sstream>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    namespace
    {

        vector<std::shared_ptr<Contract>> asContracts(
            const vector<std::shared_ptr<SingleInstrumentContract>>& instruments)
        {
            vector<std::shared_ptr<Contract>> contracts;
            contracts.reserve(instruments.size());
            for (size_t i = 0; i < instruments.size(); ++i)
            {
                contracts.push_back(instruments[i]);
            }
            return contracts;
        }

        vector<std::shared_ptr<SingleInstrumentContract>> flatten(
            const vector<std::shared_ptr<Contract>>& inputs)
        {
            vector<std::shared_ptr<SingleInstrumentContract>> instruments;
            for (size_t i = 0; i < inputs.size(); ++i)
            {
                std::shared_ptr<SingleInstrumentContract> single =
                    std::dynamic_pointer_cast<SingleInstrumentContract>(inputs[i]);
                if (single)
                {
                    instruments.push_back(single);
                    continue;
                }
                std::shared_ptr<MultipleInstrumentContract> group =
                    std::dynamic_pointer_cast<MultipleInstrumentContract>(inputs[i]);
                if (!group)
                {
                    continue;
                }
                vector<std::shared_ptr<SingleInstrumentContract>> underlying =
                    group->getUnderlyingSingleInstrumentContracts();
                for (size_t j = 0; j < underlying.size(); ++j)
                {
                    instruments.push_back(underlying[j]);
                }
            }
            return instruments;
        }

    }

    std::string CapitalModel::validationRoot() const
    {
        return "capital_model";
    }

    ValidationReport CapitalModel::validate(RatesEnvironment& re)
    {
        resetErrorMessages();
        runChecks(re);
        return validationReport();
    }

    ValidationReport CapitalModel::validate(const std::shared_ptr<RatesEnvironment>& re)
    {
        return validate(*re);
    }

    void CapitalModel::runChecks(RatesEnvironment& /*re*/) {}

    std::string CurrentExposureMethod::validationRoot() const
    {
        return "current_exposure";
    }

    CurrentExposureMethod::CurrentExposureMethod(const Portfolio& portfolio,
                                                 AssetClass asset,
                                                 CounterpartyCredit creditIn,
                                                 bool optimise)
        : CurrentExposureMethod(
              asContracts(portfolio.getUnderlyingSingleInstrumentContracts()), asset, creditIn, optimise)
    {
    }

    CurrentExposureMethod::CurrentExposureMethod(vector<std::shared_ptr<Contract>> inputs,
                                                 AssetClass asset,
                                                 CounterpartyCredit creditIn,
                                                 bool optimise)
        : assetClass(asset), credit(creditIn)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("CurrentExposureMethod");
        structureConversionError = false;
        conversionDetailText = "Netting sets not yet created";
        build(std::move(inputs), optimise);
    }

    void CurrentExposureMethod::build(vector<std::shared_ptr<Contract>> inputs, bool optimise)
    {
        if (optimise)
        {
            Result<vector<std::shared_ptr<CemNettingSet>>> built = optimiseSets(flatten(inputs), assetClass);
            nettingSets_ = built.value;
            structureConversionError = !built.ok();
            if (!built.ok())
            {
                conversionDetailText = built.report.toString();
                errorTracking->absorb(built.report, {"netting_sets"});
                SJD_LOG_DEBUG(built.report.toString());
            }
            else
            {
                size_t capped = 0;
                std::ostringstream volumes;
                for (const std::shared_ptr<CemNettingSet>& set : built.value)
                {
                    if (set->isExposureCapped())
                        ++capped;
                    if (!volumes.str().empty())
                        volumes << ',';
                    volumes << set->getVolume();
                }
                SJD_LOG_INFO("CEM optimisation netting_sets=" + std::to_string(built.value.size()) +
                             " capped=" + std::to_string(capped) + " uncapped=" +
                             std::to_string(built.value.size() - capped) + " volumes=" + volumes.str());
            }
        }
        else
        {
            for (size_t i = 0; i < inputs.size(); ++i)
            {
                std::shared_ptr<CemNettingSet> nettingSet =
                    std::make_shared<CemNettingSet>(inputs[i], assetClass);
                if (!nettingSet->isOK())
                {
                    errorTracking->absorb(nettingSet->validationReport(),
                                          {"netting_sets", std::to_string(i)});
                    structureConversionError = true;
                    conversionDetailText = "Netting set conversion failed";
                }
                else
                {
                    nettingSets_.push_back(nettingSet);
                }
            }
        }

        if (errorTracking->getHasErrors())
        {
            nettingSets_.clear();
        }
    }

    void CurrentExposureMethod::runChecks()
    {
        resetErrorMessages();
        if (structureConversionError)
        {
            errorTracking->record(issues::PORTFOLIO_STRUCTURE_CONVERSION, conversionDetailText,
                                  {"netting_sets"});
            return;
        }
        for (size_t i = 0; i < nettingSets_.size(); ++i)
        {
            nettingSets_[i]->validate();
            if (!nettingSets_[i]->isOK())
            {
                errorTracking->absorb(nettingSets_[i]->validationReport(),
                                      {"netting_sets", std::to_string(i)});
            }
        }
    }

    void CurrentExposureMethod::runChecks(RatesEnvironment& re)
    {
        resetErrorMessages();
        if (structureConversionError)
        {
            errorTracking->record(issues::PORTFOLIO_STRUCTURE_CONVERSION, conversionDetailText,
                                  {"netting_sets"});
            return;
        }
        for (size_t i = 0; i < nettingSets_.size(); ++i)
        {
            nettingSets_[i]->validate(re);
            if (!nettingSets_[i]->isOK())
            {
                errorTracking->absorb(nettingSets_[i]->validationReport(),
                                      {"netting_sets", std::to_string(i)});
                return;
            }
        }
    }

    vector<std::shared_ptr<Contract>> CurrentExposureMethod::nettingSetContracts() const
    {
        vector<std::shared_ptr<Contract>> sets;
        for (size_t i = 0; i < nettingSets_.size(); ++i)
        {
            sets.push_back(nettingSets_[i]);
        }
        return sets;
    }

    const CounterpartyCredit& CurrentExposureMethod::counterparty() const
    {
        return credit;
    }

    Result<vector<NettingSet>> CurrentExposureMethod::nettingSets() const
    {
        Result<vector<NettingSet>> result;
        if (hasErrors() || structureConversionError)
        {
            result.report = validationReport();
            if (result.report.isOK())
                result.report =
                    ValidationReport::failure(issues::PORTFOLIO_STRUCTURE_CONVERSION, conversionDetailText);
            return result;
        }
        for (size_t s = 0; s < nettingSets_.size(); ++s)
        {
            std::shared_ptr<CemNettingSet> set = nettingSets_[s];
            NettingSet described;
            described.settlement = set->getSettlementDate();
            described.volume = set->getVolume();
            described.hasPriceExposure = set->getHasPriceExposure();
            described.capped = set->isExposureCapped();
            described.cap = set->getCap();
            result.value.push_back(described);
        }
        return result;
    }

    Result<vector<std::shared_ptr<CemNettingSet>>> CurrentExposureMethod::optimiseSets(
        vector<std::shared_ptr<SingleInstrumentContract>> contractsInput, AssetClass asset)
    {
        Result<vector<std::shared_ptr<CemNettingSet>>> result;
        if (contractsInput.size() == 0)
        {
            result.report = ValidationReport::failure(issues::PORTFOLIO_STRUCTURE_CONVERSION,
                                                      "No contracts to convert into netting sets");
            return result;
        }

        list<std::shared_ptr<SingleInstrumentContract>> tmpContracts;
        for (size_t i = 0; i < contractsInput.size(); ++i)
        {
            tmpContracts.push_back(cloneInstrument(contractsInput[i]));
        }

        vector<std::shared_ptr<SingleInstrumentContract>> potentialContractsForOneSet;
        while (tmpContracts.size() > 0)
        {
            potentialContractsForOneSet.clear();
            potentialContractsForOneSet.push_back(tmpContracts.front());
            tmpContracts.erase(tmpContracts.begin());
            list<std::shared_ptr<SingleInstrumentContract>>::iterator it = tmpContracts.begin();
            while (it != tmpContracts.end())
            {
                if (CemNettingSet::qualifyForRegulatoryCapitalOffset(*it,
                                                                     potentialContractsForOneSet.front()))
                {
                    potentialContractsForOneSet.push_back(*it);
                    it = tmpContracts.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            Result<vector<std::shared_ptr<CemNettingSet>>> setsForOneSettlement =
                CemNettingSet::createStructures(potentialContractsForOneSet, asset);
            if (!setsForOneSettlement.ok())
            {
                stringstream descriptionStream;
                descriptionStream << "Unable to convert single instruments for settlement on "
                                  << QuantLib::io::iso_date(
                                         potentialContractsForOneSet[0]->getSettlementDate())
                                  << " into a netting set";
                result.report = ValidationReport::failure(issues::PORTFOLIO_STRUCTURE_CONVERSION,
                                                          descriptionStream.str());
                result.value.clear();
                return result;
            }
            for (size_t i = 0; i < setsForOneSettlement.value.size(); ++i)
            {
                result.value.push_back(setsForOneSettlement.value[i]);
            }
        }
        return result;
    }

    CapitalResult CurrentExposureMethod::capital(const Portfolio& /*portfolio*/,
                                                 const Margin* margin,
                                                 const std::shared_ptr<RatesEnvironment>& re) const
    {
        SJD_ASSERT(!hasErrors());
        vector<double> values(nettingSets_.size(), 0.0);
        double mtm = 0;
        double grossValue = 0;
        for (size_t i = 0; i < nettingSets_.size(); ++i)
        {
            values[i] = nettingSets_[i]->getValue(re);
            mtm += values[i];
            grossValue += max(0.0, values[i]);
        }
        double ngr = 0;
        if (mtm >= 0 && grossValue > 1e-14)
            ngr = mtm / grossValue;

        CapitalResult result;
        result.netGrossRatio = ngr;
        double exposure = 0;
        for (size_t i = 0; i < nettingSets_.size(); ++i)
        {
            const double mtmPlusAddOn = nettingSets_[i]->getMtMPlusAddOnForGivenMtM(re, values[i], ngr);
            exposure += mtmPlusAddOn;
            CapitalComponent component;
            component.settlement = nettingSets_[i]->getSettlementDate();
            component.volume = nettingSets_[i]->getVolume();
            component.capped = nettingSets_[i]->isExposureCapped();
            component.markToMarket = values[i];
            component.addOn = mtmPlusAddOn - values[i];
            result.components.push_back(component);
        }
        const double replacement = margin ? max(0.0, mtm - margin->getMarginBalance()) : max(0.0, mtm);
        const double unscaled = exposure - mtm + replacement;
        result.replacement = replacement;
        result.ead = max(0.0, unscaled);
        result.rwa = result.ead * credit.riskWeight;
        result.capital = max(0.0, unscaled * credit.capitalRatio * credit.riskWeight);
        return result;
    }

}
