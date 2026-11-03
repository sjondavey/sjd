#include <sjd/contracts/Portfolio/RegulatoryCapital.h>

#include <sjd/tools/Errors.h>

#include <algorithm>
#include <iomanip>
#include <limits>
#include <list>
#include <sstream>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    std::string RegulatoryCapital::validationRoot() const
    {
        return "regulatory_capital";
    }

    ValidationReport RegulatoryCapital::validate(RatesEnvironment& re)
    {
        resetErrorMessages();
        runChecks(re);
        return validationReport();
    }

    ValidationReport RegulatoryCapital::validate(const RatesEnvironment& re)
    {
        return validate(const_cast<RatesEnvironment&>(re));
    }

    ValidationReport RegulatoryCapital::validate(const std::shared_ptr<RatesEnvironment>& re)
    {
        return validate(*re);
    }

    void RegulatoryCapital::runChecks() {}

    void RegulatoryCapital::runChecks(RatesEnvironment& /*re*/) {}

    CurrentExposureMethod::CurrentExposureMethod(
        vector<std::shared_ptr<Contract>> inputs,
        vector<std::shared_ptr<SingleInstrumentContract>> extractedContracts,
        std::shared_ptr<Margin> marginInput,
        std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactorsInput,
        std::shared_ptr<RegCapCreditParameters> regCapCreditParametersInput,
        bool optimise)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("CurrentExposureMethod");
        regCapAssetFactors = regCapAssetFactorsInput;
        regCapCreditParameters = regCapCreditParametersInput;
        hasMargin = static_cast<bool>(marginInput);
        margin = marginInput;
        structureConversionError = false;
        conversionDetailText = "Structures not yet created";
        if (!regCapAssetFactors)
        {
            regCapAssetFactors = generateRegCapFactors("commodity");
        }
        if (!regCapCreditParameters)
        {
            regCapCreditParameters = std::make_shared<RegCapCreditParameters>(0, 0);
        }

        if (optimise)
        {
            Result<vector<std::shared_ptr<Structure>>> built =
                optimiseStructuresFromContracts(extractedContracts, regCapAssetFactors);
            nettingStructures = built.value;
            structureConversionError = !built.ok();
            if (!built.ok())
            {
                conversionDetailText = built.report.toString();
                errorTracking->absorb(built.report, {"regulatory_capital", "structures"});
            }
        }
        else
        {
            for (size_t i = 0; i < inputs.size(); ++i)
            {
                std::shared_ptr<Structure> nettingSet =
                    std::make_shared<Structure>(inputs[i], regCapAssetFactors);
                if (!nettingSet->isOK())
                {
                    errorTracking->absorb(nettingSet->validationReport(),
                                          {"regulatory_capital", "structures", std::to_string(i)});
                    structureConversionError = true;
                    conversionDetailText = "Structure conversion failed";
                }
                else
                {
                    nettingStructures.push_back(nettingSet);
                }
            }
        }

        if (errorTracking->getHasErrors())
        {
            nettingStructures.clear();
        }
    }

    CurrentExposureMethod::CurrentExposureMethod(const CurrentExposureMethod& other)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("CurrentExposureMethod");
        hasMargin = other.hasMargin;
        margin = other.margin;
        regCapAssetFactors = other.regCapAssetFactors;
        regCapCreditParameters = other.regCapCreditParameters;
        structureConversionError = other.structureConversionError;
        conversionDetailText = other.conversionDetailText;
        for (size_t i = 0; i < other.nettingStructures.size(); ++i)
        {
            std::unique_ptr<Contract> cloned = other.nettingStructures[i]->clone();
            nettingStructures.push_back(
                std::shared_ptr<Structure>(static_cast<Structure*>(cloned.release())));
        }
    }

    void CurrentExposureMethod::runChecks()
    {
        resetErrorMessages();
        if (structureConversionError)
        {
            errorTracking->record(issues::PORTFOLIO_STRUCTURE_CONVERSION, conversionDetailText,
                                  {"regulatory_capital", "structures"});
            return;
        }
        for (size_t i = 0; i < nettingStructures.size(); ++i)
        {
            nettingStructures[i]->validate();
            if (!nettingStructures[i]->isOK())
            {
                errorTracking->absorb(nettingStructures[i]->validationReport(),
                                      {"regulatory_capital", "structures", std::to_string(i)});
            }
        }
    }

    void CurrentExposureMethod::runChecks(RatesEnvironment& re)
    {
        resetErrorMessages();
        if (structureConversionError)
        {
            errorTracking->record(issues::PORTFOLIO_STRUCTURE_CONVERSION, conversionDetailText,
                                  {"regulatory_capital", "structures"});
            return;
        }
        for (size_t i = 0; i < nettingStructures.size(); ++i)
        {
            nettingStructures[i]->validate(re);
            if (!nettingStructures[i]->isOK())
            {
                errorTracking->absorb(nettingStructures[i]->validationReport(),
                                      {"regulatory_capital", "structures", std::to_string(i)});
                return;
            }
        }
    }

    bool CurrentExposureMethod::hasStructureConversionError() const
    {
        return structureConversionError;
    }

    const string& CurrentExposureMethod::conversionDetail() const
    {
        return conversionDetailText;
    }

    const vector<std::shared_ptr<Structure>>& CurrentExposureMethod::structures() const
    {
        return nettingStructures;
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
        vector<NettingSet>& described = result.value;
        for (size_t s = 0; s < nettingStructures.size(); ++s)
        {
            std::shared_ptr<Structure> structure = nettingStructures[s];
            NettingSet describedSet;
            describedSet.settlement = structure->getSettlementDate();
            describedSet.volume = structure->getVolume();
            describedSet.hasPriceExposure = structure->getHasPriceExposure();
            describedSet.capped = structure->isExposureCapped();
            describedSet.cap = structure->getCap();
            described.push_back(describedSet);
        }
        return result;
    }

    Result<vector<std::shared_ptr<Structure>>> CurrentExposureMethod::optimiseStructuresFromContracts(
        vector<std::shared_ptr<SingleInstrumentContract>> contractsInput,
        std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors)
    {
        Result<vector<std::shared_ptr<Structure>>> result;
        if (contractsInput.size() == 0)
        {
            result.report = ValidationReport::failure(issues::PORTFOLIO_STRUCTURE_CONVERSION,
                                                      "No contracts to convert into structures");
            return result;
        }

        // createStructures changes volume and premium, so it works on copies.
        list<std::shared_ptr<SingleInstrumentContract>> tmpContracts;
        for (size_t i = 0; i < contractsInput.size(); ++i)
        {
            tmpContracts.push_back(cloneInstrument(contractsInput[i]));
        }

        vector<std::shared_ptr<SingleInstrumentContract>> potentialContractsForOneStructure;
        while (tmpContracts.size() > 0)
        {
            potentialContractsForOneStructure.clear();
            potentialContractsForOneStructure.push_back(tmpContracts.front());
            tmpContracts.erase(tmpContracts.begin());
            list<std::shared_ptr<SingleInstrumentContract>>::iterator it = tmpContracts.begin();
            while (it != tmpContracts.end())
            {
                if (Structure::qualifyForRegulatoryCapitalOffset(*it,
                                                                 potentialContractsForOneStructure.front()))
                {
                    potentialContractsForOneStructure.push_back(*it);
                    it = tmpContracts.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            Result<vector<std::shared_ptr<Structure>>> structuresForOneSettlement =
                Structure::createStructures(potentialContractsForOneStructure, regCapAssetFactors);
            if (!structuresForOneSettlement.ok())
            {
                stringstream descriptionStream;
                descriptionStream << "Unable to convert single instruments for settlement on "
                                  << QuantLib::io::iso_date(
                                         potentialContractsForOneStructure[0]->getSettlementDate())
                                  << " into a single structure";
                result.report = ValidationReport::failure(issues::PORTFOLIO_STRUCTURE_CONVERSION,
                                                          descriptionStream.str());
                result.value.clear();
                return result;
            }
            for (size_t i = 0; i < structuresForOneSettlement.value.size(); ++i)
            {
                result.value.push_back(structuresForOneSettlement.value[i]);
            }
        }
        return result;
    }

    double CurrentExposureMethod::getNGR(std::shared_ptr<RatesEnvironment> re, double& mtm)
    {
        SJD_ASSERT(!hasErrors());
        if (!regCapAssetFactors)
        {
            return 0;
        }
        double netValue = 0, grossValue = 0, ngr = 0;
        for (size_t i = 0; i < nettingStructures.size(); ++i)
        {
            double value = nettingStructures[i]->getValue(re);
            netValue += value;
            grossValue += max(0.0, value);
        }
        mtm = netValue;
        if (netValue < 0)
        {
            ngr = 0;
        }
        else if (grossValue > 1e-14)
        {
            ngr = netValue / grossValue;
        }
        else
        {
            ngr = 0;
        }
        return ngr;
    }

    double CurrentExposureMethod::getRegulatoryCapital(std::shared_ptr<RatesEnvironment> re)
    {
        SJD_ASSERT(!hasErrors());
        double mtm = 0;
        double ngr = getNGR(re, mtm);

        double regCap = 0;
        for (size_t i = 0; i < nettingStructures.size(); ++i)
        {
            regCap += nettingStructures[i]->getMtMPlusAddOn(re, ngr);
        }
        if (hasMargin)
        {
            regCap = regCap - mtm + max(0.0, mtm - margin->getMarginBalance());
        }
        else
        {
            regCap = regCap - mtm + max(0.0, mtm);
        }
        regCap *=
            regCapCreditParameters->getCapitalCharge() * regCapCreditParameters->getCounterpartyWeighting();
        regCap = max(0.0, regCap);
        return regCap;
    }

    PortfolioB2CEM::PortfolioB2CEM(vector<std::shared_ptr<Contract>> inputs,
                                   std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactorsInput,
                                   std::shared_ptr<RegCapCreditParameters> regCapCreditParametersInput,
                                   bool optimise)
        : Portfolio(inputs)
    {
        attachCapital(inputs, regCapAssetFactorsInput, regCapCreditParametersInput, optimise);
    }

    PortfolioB2CEM::PortfolioB2CEM(vector<std::shared_ptr<Contract>> inputs,
                                   std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactorsInput,
                                   std::shared_ptr<RegCapCreditParameters> regCapCreditParametersInput,
                                   bool optimise,
                                   std::shared_ptr<Margin> marginInput)
        : Portfolio(inputs, marginInput)
    {
        attachCapital(inputs, regCapAssetFactorsInput, regCapCreditParametersInput, optimise);
    }

    PortfolioB2CEM::PortfolioB2CEM(PortfolioB2CEM& portfolio)
    {
        setInputVariables();
        vector<std::shared_ptr<SingleInstrumentContract>> tmpContracts =
            portfolio.getUnderlyingSingleInstrumentContracts();
        for (size_t i = 0; i < tmpContracts.size(); ++i)
        {
            contracts.push_back(cloneInstrument(tmpContracts[i]));
        }
        margin = portfolio.margin;
        hasMargin = portfolio.hasMargin;
        if (portfolio.capital)
        {
            capital = std::make_shared<CurrentExposureMethod>(*portfolio.capital);
        }
    }

    std::unique_ptr<Contract> PortfolioB2CEM::clone()
    {
        return std::make_unique<PortfolioB2CEM>(*this);
    }

    void PortfolioB2CEM::attachCapital(vector<std::shared_ptr<Contract>> inputs,
                                       std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactorsInput,
                                       std::shared_ptr<RegCapCreditParameters> regCapCreditParametersInput,
                                       bool optimise)
    {
        if (errorTracking->getHasErrors())
        {
            return;
        }
        capital = std::make_shared<CurrentExposureMethod>(
            inputs, contracts, hasMargin ? margin : std::shared_ptr<Margin>(), regCapAssetFactorsInput,
            regCapCreditParametersInput, optimise);
        if (!capital->isOK())
        {
            absorbChildReport(capital->validationReport(), {"regulatory_capital"});
            contracts = vector<std::shared_ptr<SingleInstrumentContract>>(0);
        }
    }

    void PortfolioB2CEM::runChecks()
    {
        // Portfolio::validate clears the tracker, so it must run before the structure checks.
        Portfolio::runChecks();
        if (capital && capital->hasStructureConversionError())
        {
            recordIssue(issues::PORTFOLIO_STRUCTURE_CONVERSION, capital->conversionDetail(), {"structures"});
            return;
        }
        if (capital)
        {
            const vector<std::shared_ptr<Structure>>& nettingStructures = capital->structures();
            for (size_t i = 0; i < nettingStructures.size(); ++i)
            {
                nettingStructures[i]->validate();
                if (!nettingStructures[i]->isOK())
                {
                    absorbChildReport(nettingStructures[i]->validationReport(),
                                      {"structures", std::to_string(i)});
                }
            }
        }
    }

    void PortfolioB2CEM::runChecks(RatesEnvironment& re)
    {
        Portfolio::runChecks(re);
        if (capital && capital->hasStructureConversionError())
        {
            recordIssue(issues::PORTFOLIO_STRUCTURE_CONVERSION, capital->conversionDetail(), {"structures"});
            return;
        }
        if (capital)
        {
            const vector<std::shared_ptr<Structure>>& nettingStructures = capital->structures();
            for (size_t i = 0; i < nettingStructures.size(); ++i)
            {
                nettingStructures[i]->validate(re);
                if (!nettingStructures[i]->isOK())
                {
                    absorbChildReport(nettingStructures[i]->validationReport(),
                                      {"structures", std::to_string(i)});
                    return;
                }
            }
        }
    }

    std::shared_ptr<RegulatoryCapital> PortfolioB2CEM::regulatoryCapital() const
    {
        return capital;
    }

    double PortfolioB2CEM::getRegulatoryCapital(std::shared_ptr<RatesEnvironment> re)
    {
        if (!capital)
        {
            return 0;
        }
        return capital->getRegulatoryCapital(re);
    }

    vector<std::shared_ptr<Contract>> PortfolioB2CEM::regulatoryStructures() const
    {
        vector<std::shared_ptr<Contract>> sets;
        if (!capital)
        {
            return sets;
        }
        const vector<std::shared_ptr<Structure>>& nettingStructures = capital->structures();
        for (size_t i = 0; i < nettingStructures.size(); ++i)
        {
            sets.push_back(nettingStructures[i]);
        }
        return sets;
    }

    Result<vector<NettingSet>> PortfolioB2CEM::nettingSets()
    {
        if (!capital)
        {
            Result<vector<NettingSet>> result;
            result.report = ValidationReport::failure(issues::PORTFOLIO_STRUCTURE_CONVERSION,
                                                      "Portfolio has no regulatory capital model");
            return result;
        }
        return capital->nettingSets();
    }

}
