#include <sjd/contracts/Portfolio/RegulatoryCapital.h>

#include <algorithm>
#include <iomanip>
#include <limits>
#include <list>
#include <sstream>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    bool RegulatoryCapital::isOK()
    {
        return errorTracking && !errorTracking->getHasErrors();
    }

    void RegulatoryCapital::validate()
    {
    }

    void RegulatoryCapital::validate(boost::shared_ptr<RatesEnvironment> /*re*/)
    {
    }

    ValidationReport RegulatoryCapital::validationReport() const
    {
        if (!errorTracking)
        {
            return ValidationReport(std::vector<ValidationIssue>());
        }
        return errorTracking->snapshot();
    }

    CurrentExposureMethod::CurrentExposureMethod(
        vector<boost::shared_ptr<Contract>> inputs,
        vector<boost::shared_ptr<SingleInstrumentContract>> extractedContracts,
        boost::shared_ptr<Margin> marginInput,
        boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactorsInput,
        boost::shared_ptr<RegCapCreditParameters> regCapCreditParametersInput,
        bool optimise)
    {
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("CurrentExposureMethod"));
        regCapAssetFactors = regCapAssetFactorsInput;
        regCapCreditParameters = regCapCreditParametersInput;
        hasMargin = static_cast<bool>(marginInput);
        margin = marginInput;
        structureConversionError = false;
        structureErrorMessage = "Structures not yet created";
        if (!regCapAssetFactors)
        {
            regCapAssetFactors = generateRegCapFactors("commodity");
        }
        if (!regCapCreditParameters)
        {
            regCapCreditParameters = boost::shared_ptr<RegCapCreditParameters>(new RegCapCreditParameters(0, 0));
        }

        if (optimise)
        {
            bool successful = false;
            nettingStructures = optimiseStructuresFromContracts(
                extractedContracts, regCapAssetFactors, successful, structureErrorMessage);
            structureConversionError = !successful;
            if (!successful)
            {
                errorTracking->record("PORTFOLIO_STRUCTURE_CONVERSION",
                                      structureErrorMessage,
                                      {"regulatory_capital", "structures"});
            }
        }
        else
        {
            for (size_t i = 0; i < inputs.size(); ++i)
            {
                boost::shared_ptr<Structure> nettingSet =
                    boost::shared_ptr<Structure>(new Structure(inputs[i], regCapAssetFactors));
                if (!nettingSet->isOK())
                {
                    errorTracking->absorb(nettingSet->validationReport(),
                                          {"regulatory_capital", "structures", std::to_string(i)});
                    structureConversionError = true;
                    structureErrorMessage = "Structure conversion failed";
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
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("CurrentExposureMethod"));
        hasMargin = other.hasMargin;
        margin = other.margin;
        regCapAssetFactors = other.regCapAssetFactors;
        regCapCreditParameters = other.regCapCreditParameters;
        structureConversionError = other.structureConversionError;
        structureErrorMessage = other.structureErrorMessage;
        for (size_t i = 0; i < other.nettingStructures.size(); ++i)
        {
            std::unique_ptr<Contract> cloned = other.nettingStructures[i]->clone();
            nettingStructures.push_back(
                boost::shared_ptr<Structure>(static_cast<Structure*>(cloned.release())));
        }
    }

    bool CurrentExposureMethod::isOK()
    {
        return RegulatoryCapital::isOK();
    }

    void CurrentExposureMethod::validate()
    {
        if (structureConversionError)
        {
            errorTracking->record("PORTFOLIO_STRUCTURE_CONVERSION",
                                  structureErrorMessage,
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

    void CurrentExposureMethod::validate(boost::shared_ptr<RatesEnvironment> re)
    {
        if (structureConversionError)
        {
            errorTracking->record("PORTFOLIO_STRUCTURE_CONVERSION",
                                  structureErrorMessage,
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

    ValidationReport CurrentExposureMethod::validationReport() const
    {
        return RegulatoryCapital::validationReport();
    }

    bool CurrentExposureMethod::hasStructureConversionError() const
    {
        return structureConversionError;
    }

    const string& CurrentExposureMethod::structureConversionErrorMessage() const
    {
        return structureErrorMessage;
    }

    const vector<boost::shared_ptr<Structure>>& CurrentExposureMethod::structures() const
    {
        return nettingStructures;
    }

    vector<boost::shared_ptr<Structure>> CurrentExposureMethod::optimiseStructuresFromContracts(
        vector<boost::shared_ptr<SingleInstrumentContract>> contractsInput,
        boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
        bool& sucessfullportfolio,
        string& errorMessage)
    {
        sucessfullportfolio = true;
        errorMessage = "No error converting structures to portfolio";
        if (contractsInput.size() == 0)
        {
            sucessfullportfolio = false;
            errorMessage = "No contracts to convert into structures";
            return vector<boost::shared_ptr<Structure>>(0);
        }

        list<boost::shared_ptr<SingleInstrumentContract>> tmpContracts;
        for (size_t i = 0; i < contractsInput.size(); ++i)
        {
            tmpContracts.push_back(cloneInstrument(contractsInput[i]));
        }

        vector<boost::shared_ptr<SingleInstrumentContract>> potentialContractsForOneStructure;
        vector<boost::shared_ptr<Structure>> allStructures;
        while (tmpContracts.size() > 0)
        {
            potentialContractsForOneStructure.clear();
            potentialContractsForOneStructure.push_back(tmpContracts.front());
            tmpContracts.erase(tmpContracts.begin());
            list<boost::shared_ptr<SingleInstrumentContract>>::iterator it = tmpContracts.begin();
            while (it != tmpContracts.end())
            {
                if (Structure::qualifyForRegulatoryCapitalOffset(*it, potentialContractsForOneStructure.front()))
                {
                    potentialContractsForOneStructure.push_back(*it);
                    it = tmpContracts.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            bool successful = false;
            vector<boost::shared_ptr<Structure>> structuresForOneSettlement =
                Structure::createStructures(potentialContractsForOneStructure, regCapAssetFactors, successful);
            if (!successful)
            {
                sucessfullportfolio = false;
                stringstream descriptionStream;
                descriptionStream << "Unable to convert single instruments for settlement on "
                                  << QuantLib::io::iso_date(potentialContractsForOneStructure[0]->getSettlementDate())
                                  << " into a single structure";
                errorMessage = descriptionStream.str();
            }
            for (size_t i = 0; i < structuresForOneSettlement.size(); ++i)
            {
                allStructures.push_back(structuresForOneSettlement[i]);
            }
        }
        return allStructures;
    }

    double CurrentExposureMethod::getNGR(boost::shared_ptr<RatesEnvironment> re, double& mtm)
    {
        if (errorTracking->getHasErrors())
        {
            numeric_limits<double>::quiet_NaN();
        }
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

    double CurrentExposureMethod::getRegulatoryCapital(boost::shared_ptr<RatesEnvironment> re)
    {
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
        regCap *= regCapCreditParameters->getCapitalCharge() * regCapCreditParameters->getCounterpartyWeighting();
        regCap = max(0.0, regCap);
        return regCap;
    }

    PortfolioB2CEM::PortfolioB2CEM(vector<boost::shared_ptr<Contract>> inputs,
                                   boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactorsInput,
                                   boost::shared_ptr<RegCapCreditParameters> regCapCreditParametersInput,
                                   bool optimise)
        : Portfolio(inputs)
    {
        attachCapital(inputs, regCapAssetFactorsInput, regCapCreditParametersInput, optimise);
    }

    PortfolioB2CEM::PortfolioB2CEM(vector<boost::shared_ptr<Contract>> inputs,
                                   boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactorsInput,
                                   boost::shared_ptr<RegCapCreditParameters> regCapCreditParametersInput,
                                   bool optimise,
                                   boost::shared_ptr<Margin> marginInput)
        : Portfolio(inputs, marginInput)
    {
        attachCapital(inputs, regCapAssetFactorsInput, regCapCreditParametersInput, optimise);
    }

    PortfolioB2CEM::PortfolioB2CEM(PortfolioB2CEM& portfolio)
    {
        setInputVariables();
        vector<boost::shared_ptr<SingleInstrumentContract>> tmpContracts =
            portfolio.getUnderlyingSingleInstrumentContracts();
        for (size_t i = 0; i < tmpContracts.size(); ++i)
        {
            contracts.push_back(cloneInstrument(tmpContracts[i]));
        }
        margin = portfolio.margin;
        hasMargin = portfolio.hasMargin;
        if (portfolio.capital)
        {
            capital = boost::shared_ptr<CurrentExposureMethod>(new CurrentExposureMethod(*portfolio.capital));
        }
    }

    std::unique_ptr<Contract> PortfolioB2CEM::clone()
    {
        return std::unique_ptr<Contract>(new PortfolioB2CEM(*this));
    }

    void PortfolioB2CEM::attachCapital(vector<boost::shared_ptr<Contract>> inputs,
                                       boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactorsInput,
                                       boost::shared_ptr<RegCapCreditParameters> regCapCreditParametersInput,
                                       bool optimise)
    {
        if (errorTracking->getHasErrors())
        {
            return;
        }
        capital = boost::shared_ptr<CurrentExposureMethod>(new CurrentExposureMethod(
            inputs,
            contracts,
            hasMargin ? margin : boost::shared_ptr<Margin>(),
            regCapAssetFactorsInput,
            regCapCreditParametersInput,
            optimise));
        if (!capital->isOK())
        {
            absorbChildReport(capital->validationReport(), {"regulatory_capital"});
            contracts = vector<boost::shared_ptr<SingleInstrumentContract>>(0);
        }
    }

    void PortfolioB2CEM::validate()
    {
        if (capital && capital->hasStructureConversionError())
        {
            recordIssue("PORTFOLIO_STRUCTURE_CONVERSION",
                        capital->structureConversionErrorMessage(),
                        {"structures"});
            return;
        }
        if (capital)
        {
            const vector<boost::shared_ptr<Structure>>& nettingStructures = capital->structures();
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
        Portfolio::validate();
    }

    void PortfolioB2CEM::validate(boost::shared_ptr<RatesEnvironment> re)
    {
        if (capital && capital->hasStructureConversionError())
        {
            recordIssue("PORTFOLIO_STRUCTURE_CONVERSION",
                        capital->structureConversionErrorMessage(),
                        {"structures"});
            return;
        }
        if (capital)
        {
            const vector<boost::shared_ptr<Structure>>& nettingStructures = capital->structures();
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
        Portfolio::validate(re);
    }

    boost::shared_ptr<RegulatoryCapital> PortfolioB2CEM::regulatoryCapital() const
    {
        return capital;
    }

    double PortfolioB2CEM::getRegulatoryCapital(boost::shared_ptr<RatesEnvironment> re)
    {
        if (!capital)
        {
            return 0;
        }
        return capital->getRegulatoryCapital(re);
    }

}
