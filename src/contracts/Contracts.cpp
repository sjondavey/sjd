#include <boost/lexical_cast.hpp>
#include <sjd/contracts/Contracts.h>

#include <iomanip>
#include <sstream>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    /*======================================================================================
    Contract

     =======================================================================================*/
    std::string Contract::validationRoot() const
    {
        return "contract";
    }

    void Contract::validate()
    {
        errorTracking->clear();
    }

    void Contract::validate(std::shared_ptr<RatesEnvironment> re)
    {
        this->validate();
        if (!re->isOK())
        {
            absorbChildReport(re->validationReport(), {});
        }
    }

    bool Contract::isOK()
    {
        return !hasErrors();
    }

    ValidationReport Contract::validateReport()
    {
        validate();
        return validationReport();
    }

    ValidationReport Contract::validateReport(std::shared_ptr<RatesEnvironment> re)
    {
        validate(re);
        return validationReport();
    }

    string Contract::toString()
    {
        stringstream description;
        validate();
        if (!isOK())
        {
            description << "Contract has errors,";
            vector<string> errorMessages = getErrorMessages();
            for (size_t i = 0; i < errorMessages.size(); ++i)
            {
                description << errorMessages[i] << ",";
            }
            return description.str();
        }
        description << toStringContract();
        return description.str();
    }

    string Contract::toString(std::shared_ptr<RatesEnvironment> re)
    {
        validate(re);
        stringstream description;
        if (!isOK())
        {
            description << "Contract has errors,";
            vector<string> errorMessages = getErrorMessages();
            for (size_t i = 0; i < errorMessages.size(); ++i)
            {
                description << errorMessages[i] << ",";
            }
            return description.str();
        }
        description << toStringContract();
        description << toStringRates(re);
        return description.str();
    }

    /*======================================================================================
    SingleInstrumentContract

    =======================================================================================*/
    void SingleInstrumentContract::validate()
    {
        Contract::validate();
        if (maturityDate > settlementDate)
        {
            recordIssue("CONTRACT_MATURITY_AFTER_SETTLEMENT", "Maturity date after settlement date",
                        {"maturity_date"});
        }
        if (hasPremium && (getPremiumPaymentDate() > getSettlementDate()))
        {
            recordIssue("CONTRACT_PREMIUM_DATE_AFTER_SETTLEMENT",
                        "Premium payment date after settlement date. Consider using a cash flow and an "
                        "option without premium",
                        {"premium_payment_date"});
        }
        if (hasPremium && (premiumAmount < -1e-14))
        {
            recordIssue("CONTRACT_PREMIUM_NEGATIVE", "Premium Amount must be stored as a non-negative number",
                        {"premium_amount"});
        }
    }

    void SingleInstrumentContract::validate(std::shared_ptr<RatesEnvironment> re)
    {
        Contract::validate(re);
        if (!re->hasDRS())
        {
            recordIssue("MARKET_DISCOUNT_RATE_SOURCE_MISSING", "No discounting rate source",
                        {"rates_environment", "discount_rate_source"});
        }
        else
        {
            // can't use if (!drs->isInRange(settlementDate))
            // because there are legitimate instances where we may value the option after settlement
            if (!(re->getDRS())->extendsTo(settlementDate))
            {
                recordIssue("MARKET_DISCOUNT_RATE_SOURCE_RANGE",
                            "Discounting rate source does not extend to settlement",
                            {"rates_environment", "discount_rate_source"});
            }
        }
    }

    bool SingleInstrumentContract::getHasPremium() const
    {
        return hasPremium;
    }

    Date SingleInstrumentContract::getPremiumPaymentDate() const
    {
        return premiumPaymentDate;
    }

    void SingleInstrumentContract::setPremiumPaymentDate(Date paymentDate)
    {
        premiumPaymentDate = paymentDate;
        if (premiumAmount > 1e-14)
        {
            hasPremium = true;
        }
        else
        {
            hasPremium = false;
        }
        valueCalculated = false;
    }

    void SingleInstrumentContract::setPremiumAmount(double premium)
    {
        premiumAmount = abs(premium);
        if (premiumAmount > 1e-14)
        {
            hasPremium = true;
        }
        else
        {
            hasPremium = false;
        }
        valueCalculated = false;
    }

    double SingleInstrumentContract::getPremiumAmount() const
    {
        return premiumAmount;
    }

    void SingleInstrumentContract::setMaturityDate(Date date)
    {
        maturityDate = date;
        valueCalculated = false;
    }

    Date SingleInstrumentContract::getMaturityDate() const
    {
        return maturityDate;
    }

    void SingleInstrumentContract::setSettlementDate(Date date)
    {
        settlementDate = date;
        valueCalculated = false;
    }

    Date SingleInstrumentContract::getSettlementDate() const
    {
        return settlementDate;
    }

    void SingleInstrumentContract::setBuySell(BuySell bs)
    {
        buySell = bs;
        if (buySell == BUY)
        {
            buySellAsDouble = 1;
        }
        else if (buySell == SELL)
        {
            buySellAsDouble = -1;
        }
        else
        {
            buySellAsDouble = 0;
        }
        valueCalculated = false;
    }

    BuySell SingleInstrumentContract::getBuySell() const
    {
        return buySell;
    }

    double SingleInstrumentContract::getBuySellAsDouble() const
    {
        return buySellAsDouble;
    }

    double SingleInstrumentContract::getVolume() const
    {
        return volume;
    }

    void SingleInstrumentContract::setVolume(double N)
    {
        volume = abs(N);
        valueCalculated = false;
    }

    string SingleInstrumentContract::toStringContract()
    {
        stringstream descriptionStream;
        descriptionStream << errorTracking->getClassName() << ",Set,"
                          << QuantLib::io::iso_date(getSettlementDate()) << ",Mat,"
                          << QuantLib::io::iso_date(getMaturityDate()) << ","
                          << convertBuySellToString(buySell) << ",Volume," << fixed << setprecision(2)
                          << volume;
        if (hasPremium)
        {
            descriptionStream << ",Prem," << fixed << setprecision(4) << premiumAmount << ",PremDate,"
                              << QuantLib::io::iso_date(getPremiumPaymentDate());
        }
        else
        {
            descriptionStream << ",NoPrem,0,PremDate,01/01/1901";
        }
        return descriptionStream.str();
    }

    string SingleInstrumentContract::toStringRates(std::shared_ptr<RatesEnvironment> re)
    {
        stringstream ratesDescriptionStream;
        ratesDescriptionStream << ",MtM," << fixed << setprecision(4) << getValue(re) << ",DF(set)," << fixed
                               << setprecision(6) << (re->getDRS())->getDiscountFactor(getSettlementDate());
        return ratesDescriptionStream.str();
    }

    /*======================================================================================
    SingleDerivativeContract

    =======================================================================================*/
    void SingleDerivativeContract::validate()
    {
        SingleInstrumentContract::validate();
        if (strike < 0)
        {
            recordIssue("CONTRACT_STRIKE_NEGATIVE", "Strike is negative", {"strike"});
        }
    }

    void SingleDerivativeContract::validate(std::shared_ptr<RatesEnvironment> re)
    {
        SingleInstrumentContract::validate(re);
        if (!re->hasFRS())
        {
            recordIssue("MARKET_FORWARD_RATE_SOURCE_MISSING", "No forward rate source",
                        {"rates_environment", "forward_rate_source"});
        }
        else
        {
            // NB This will need to be overwritten for Asian derivaitves which only require fwds to the
            // averaging dates
            if (!(re->getFRS())->extendsTo(getSettlementDate()))
            {
                recordIssue("MARKET_FORWARD_RATE_SOURCE_RANGE",
                            "Contract settlement not in forward rate source range",
                            {"rates_environment", "forward_rate_source"});
            }
        }
    }

    void SingleDerivativeContract::setStrike(double X)
    {
        strike = X;
        valueCalculated = false;
    }

    double SingleDerivativeContract::getStrike() const
    {
        return strike;
    }

    string SingleDerivativeContract::toStringContract()
    {
        stringstream descriptionStream;
        descriptionStream << SingleInstrumentContract::toStringContract();
        descriptionStream << ",Strike," << fixed << setprecision(4) << strike;
        return descriptionStream.str();
    }

    string SingleDerivativeContract::toStringRates(std::shared_ptr<RatesEnvironment> re)
    {
        stringstream descriptionStream;
        descriptionStream << SingleInstrumentContract::toStringRates(re);
        if (isOK())
        {
            descriptionStream << ",Fwd(Set)," << fixed << setprecision(4)
                              << (re->getFRS())->getForward(getSettlementDate());
        }
        return descriptionStream.str();
    }

    /*======================================================================================
    SingleOptionContract

    =======================================================================================*/
    void SingleOptionContract::validate()
    {
        SingleDerivativeContract::validate();
        if ((type != CALL) && (type != PUT))
        {
            recordIssue("OPTION_TYPE_INVALID", "Type must be either CALL or PUT", {"put_call"});
        }
    }

    void SingleOptionContract::validate(std::shared_ptr<RatesEnvironment> re)
    {
        SingleDerivativeContract::validate(re);
        if (!re->hasVRS())
        {
            recordIssue("MARKET_VOLATILITY_RATE_SOURCE_MISSING", "No volatiltiy rate source",
                        {"rates_environment", "volatility_rate_source"});
        }
        else
        {
            if (!(re->getVRS())->extendsTo(getSettlementDate()))
            {
                recordIssue("MARKET_VOLATILITY_RATE_SOURCE_RANGE",
                            "Contract settlement not in volatility rate source range",
                            {"rates_environment", "volatility_rate_source"});
            }
        }
    }

    double SingleOptionContract::getPayoff(double forSpot)
    {
        double premium = 0;
        if (getPremiumPaymentDate() > getMaturityDate())
        {
            premium = getPremiumAmount();
        }

        if (type == PUT)
        {
            return (max(0.0, getStrike() - forSpot) - premium) * getVolume() * getBuySellAsDouble();
        }
        else
        {
            return (max(0.0, forSpot - getStrike()) - premium) * getVolume() * getBuySellAsDouble();
        }
    }

    double SingleOptionContract::getValue(std::shared_ptr<RatesEnvironment> re)
    {
        if (valueCalculated && (re == cachedRE) && (re->marketRevision() == cachedMarketRevision))
        {
            return cachedContractValue;
        }
        Date valueDate = (re->getAnchorDate());
        if (valueDate > getSettlementDate())
        {
            return 0;
        }
        double premium = getOptionValue(re);
        if (valueDate <= getPremiumPaymentDate())
        {
            double df = (re->getDRS())->getDiscountFactor(getPremiumPaymentDate());
            premium -= getPremiumAmount() * getVolume() * df;
        }
        cachedRE = re;
        cachedMarketRevision = re->marketRevision();
        valueCalculated = true;
        cachedContractValue = getBuySellAsDouble() * premium;
        return cachedContractValue;
    }

    void SingleOptionContract::setType(PutCall pc)
    {
        type = pc;
        valueCalculated = false;
    }

    PutCall SingleOptionContract::getType() const
    {
        return type;
    }

    string SingleOptionContract::toStringContract()
    {
        stringstream descriptionStream;
        descriptionStream << SingleDerivativeContract::toStringContract();
        descriptionStream << "," << convertPutCallToString(type);
        return descriptionStream.str();
    }

    string SingleOptionContract::toStringRates(std::shared_ptr<RatesEnvironment> re)
    {
        stringstream descriptionStream;
        descriptionStream << SingleDerivativeContract::toStringRates(re);
        if (isOK())
        {
            descriptionStream << ",Vol(Mat)," << fixed << setprecision(6)
                              << (re->getVRS())->getVolatility(getMaturityDate());
        }
        return descriptionStream.str();
    }

    /*======================================================================================
    AsianContract

    =======================================================================================*/
    void AsianContract::validate()
    {
        if (orderedAveragingDates.size() < 1)
        {
            recordAsianIssue("ASIAN_AVERAGING_DATES_MISSING", "Averaging Dates not set", {"averaging_dates"});
        }
    }

    void AsianContract::validate(std::shared_ptr<RatesEnvironment> re)
    {
        // Averaging dates are checked by validate(), which the instrument's
        // validate(re) has already run. Recording them again would duplicate
        // the issue on the shared tracker.
        if (orderedAveragingDates.empty())
        {
            return;
        }
        Date valueDate = (re->getAnchorDate());
        if (!re->hasFixingRS())
        {
            recordAsianIssue("MARKET_FIXING_RATE_SOURCE_MISSING", "No Fixing rate source",
                             {"rates_environment", "fixing_rate_source"});
        }
        else if (!(re->getFixingRS())->isOK())
        {
            absorbAsianReport(re->getFixingRS()->validationReport(),
                              {"rates_environment", "fixing_rate_source"});
        }

        vector<Date> fixingDates, forwardDates;
        getFixingAndForwardDates(valueDate, fixingDates, forwardDates);

        if (!re->hasFixingRS())
        {
            return;
        }
        for (size_t i = 0; i < fixingDates.size(); ++i)
        {
            Date d = fixingDates[i];
            if (!(re->getFixingRS())->containsARateSetOn(d))
            {
                stringstream descriptionStream;
                descriptionStream << "Fixing Rate Source does not contain a fixing rate on "
                                  << QuantLib::io::iso_date(d);
                recordAsianIssue("ASIAN_FIXING_DATE_MISSING", descriptionStream.str(),
                                 {"rates_environment", "fixing_rate_source", "fixing_dates"});
            }
        }
    }

    void AsianContract::setAveragingDates(set<Date> dates)
    {
        orderedAveragingDates = vector<Date>(dates.begin(), dates.end());
    }

    vector<Date> AsianContract::getAveragingDates() const
    {
        return orderedAveragingDates;
    }

    set<Date> AsianContract::getAllBusinessDaysBetween(Date startDate, Date endDate, Calendar calendar)
    {
        Date averagingStart;
        if (calendar.isBusinessDay(startDate))
        {
            averagingStart = startDate;
        }
        else
        {
            averagingStart = calendar.advance(startDate, 1, Days);
        }
        Date lastDateInAveragingMonth;
        if (calendar.isBusinessDay(endDate))
        {
            lastDateInAveragingMonth = endDate;
        }
        else
        {
            lastDateInAveragingMonth = calendar.advance(endDate, -1, Days);
        }
        set<Date> averagingDates;
        Date tmp = averagingStart;
        while (tmp <= endDate)
        {
            averagingDates.insert(tmp);
            tmp = calendar.advance(tmp, 1, Days);
        }
        return averagingDates;
    }

    void AsianContract::getFixingAndForwardDates(Date valuationDate,
                                                 vector<Date>& fixingDates,
                                                 vector<Date>& forwardDates)
    {
        if (valuationDate < orderedAveragingDates[0])
        {
            forwardDates = orderedAveragingDates;
            fixingDates = vector<Date>(0);
            return;
        }
        fixingDates.clear();
        forwardDates.clear();
        for (size_t i = 0; i < orderedAveragingDates.size(); ++i)
        {
            if (orderedAveragingDates[i] <= valuationDate)
            {
                fixingDates.push_back(orderedAveragingDates[i]);
            }
            else
            {
                forwardDates.push_back(orderedAveragingDates[i]);
            }
        }
    }

    void AsianContract::getFixingAndForwardRates(Date valueDate,
                                                 vector<double>& fixingRates,
                                                 vector<double>& forwardRates,
                                                 std::shared_ptr<RatesEnvironment> re)
    {
        vector<Date> fixingDates, forwardDates;
        getFixingAndForwardDates(valueDate, fixingDates, forwardDates);
        fixingRates = (re->getFixingRS())->getRateSetOn(fixingDates);
        forwardRates = (re->getFRS())->getForward(forwardDates);
        return;
    }

    /*======================================================================================
     MultipleInstrumentContract

     Base class for derivatives may contain more than one underlying contract
     =======================================================================================*/
    MultipleInstrumentContract::MultipleInstrumentContract(
        MultipleInstrumentContract& multipleInstrumentContract)
    {
        setInputVariables();
        contracts = multipleInstrumentContract.contracts;
    }

    std::unique_ptr<Contract> MultipleInstrumentContract::clone()
    {
        return std::unique_ptr<Contract>(new MultipleInstrumentContract(*this));
    }
    void MultipleInstrumentContract::setInputVariables()
    {
        errorTracking =
            std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("MultipleInstrumentContract"));
    }

    MultipleInstrumentContract::MultipleInstrumentContract(
        vector<std::shared_ptr<SingleInstrumentContract>> inputs)
    {
        setInputVariables();
        contracts = inputs;
    };

    MultipleInstrumentContract::MultipleInstrumentContract(vector<std::shared_ptr<Contract>> inputs)
    {
        setInputVariables();
        vector<std::shared_ptr<SingleInstrumentContract>> inputsAsSingleInstruments =
            getSingleInstrumentContracts(inputs);
        if (!isOK())
        {
            contracts = vector<std::shared_ptr<SingleInstrumentContract>>(0);
            recordIssue("CONTRACT_EXTRACT_FAILED", "Cannot extract single instruments from input contracts",
                        {});
        }
        else
        {
            contracts = inputsAsSingleInstruments;
        }
    };

    void MultipleInstrumentContract::validate()
    {
        Contract::validate();
        if (contracts.size() == 0)
        {
            recordIssue("PORTFOLIO_EMPTY", ", contains no contracts", {});
            return;
        }

        for (size_t i = 0; i < contracts.size(); ++i)
        {
            contracts[i]->validate();
            if (!contracts[i]->isOK())
            {
                absorbChildReport(contracts[i]->validationReport(),
                                  {"contracts[" + boost::lexical_cast<std::string>(i) + "]"});
            }
        }
    }

    void MultipleInstrumentContract::validate(std::shared_ptr<RatesEnvironment> re)
    {
        Contract::validate(re);
        if (contracts.size() == 0)
        {
            recordIssue("PORTFOLIO_EMPTY", ", contains no contracts", {});
            return;
        }
        for (size_t i = 0; i < contracts.size(); ++i)
        {
            contracts[i]->validate(re);
            if (!contracts[i]->isOK())
            {
                absorbChildReport(contracts[i]->validationReport(),
                                  {"contracts[" + boost::lexical_cast<std::string>(i) + "]"});
            }
        }
    }

    double MultipleInstrumentContract::getValue(std::shared_ptr<RatesEnvironment> re)
    {
        double value = 0;
        for (size_t i = 0; i < contracts.size(); ++i)
        {
            value += contracts[i]->getValue(re);
        }
        return value;
    }

    string MultipleInstrumentContract::toString(std::shared_ptr<RatesEnvironment> re)
    {
        validate(re);
        stringstream description;
        if (!isOK())
        {
            description << "Contract has errors,";
            vector<string> errorMessages = getErrorMessages();
            for (size_t i = 0; i < errorMessages.size(); ++i)
            {
                description << errorMessages[i] << ",";
            }
            return description.str();
        }
        description << "Net MtM," << fixed << setprecision(4) << getValue(re) << "\n";
        description << toStringRates(re);
        return description.str();
    }

    string MultipleInstrumentContract::toStringContract()
    {
        string description;
        for (size_t i = 0; i < contracts.size(); ++i)
        {
            description += contracts[i]->toStringContract() + "\n";
        }
        return description;
    }

    string MultipleInstrumentContract::toStringRates(std::shared_ptr<RatesEnvironment> re)
    {
        string description;
        for (size_t i = 0; i < contracts.size(); ++i)
        {
            description += contracts[i]->toStringContract() + contracts[i]->toStringRates(re) + "\n";
        }
        return description;
    }

    vector<std::shared_ptr<SingleInstrumentContract>> MultipleInstrumentContract::
        getUnderlyingSingleInstrumentContracts()
    {
        return contracts;
    }

    vector<std::shared_ptr<SingleInstrumentContract>> MultipleInstrumentContract::
        getSingleInstrumentContracts(vector<std::shared_ptr<Contract>> inputs)
    {
        vector<std::shared_ptr<SingleInstrumentContract>> inputsAsSingleInstruments;
        for (size_t i = 0; i < inputs.size(); ++i)
        {
            if (std::dynamic_pointer_cast<SingleInstrumentContract>(inputs[i]) != 0)
            {
                inputsAsSingleInstruments.push_back(
                    std::dynamic_pointer_cast<SingleInstrumentContract>(inputs[i]));
            }
            else if (std::dynamic_pointer_cast<MultipleInstrumentContract>(inputs[i]) != 0)
            {
                vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts =
                    std::dynamic_pointer_cast<MultipleInstrumentContract>(inputs[i])
                        ->getUnderlyingSingleInstrumentContracts();

                for (size_t j = 0; j < underlyingContracts.size(); ++j)
                {
                    inputsAsSingleInstruments.push_back(underlyingContracts[j]);
                }
            }
        }
        return inputsAsSingleInstruments;
    }

}