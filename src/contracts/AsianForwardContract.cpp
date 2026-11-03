#include <sjd/contracts/AsianForwardContract.h>

#include <sjd/tools/Errors.h>

#include <iomanip>
#include <sstream>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    AsianForward

     =======================================================================================*/
    void AsianForward::constructorCommon(set<Date> averagingDatesInput,
                                         Date settlementInput,
                                         double strikeInput,
                                         double volumeInput,
                                         BuySell buySellInput)
    {
        ForwardContract::errorTracking = std::make_shared<sjd::IssueTracker>("AsianForward");
        setAveragingDates(averagingDatesInput);
        setSettlementDate(settlementInput);
        setStrike(strikeInput);
        setVolume(volumeInput);
        setBuySell(buySellInput);
        setPremiumAmount(0);
        setPremiumPaymentDate(settlementInput);
    }

    AsianForward::AsianForward(
        set<Date> averagingDates, Date settlement, double strike, double volume, BuySell buySell)
    {
        constructorCommon(averagingDates, settlement, strike, volume, buySell);
    }

    AsianForward::AsianForward(Calendar calendar,
                               Date startOfAveraging,
                               Date endOfAveraging,
                               Date settlement,
                               double strike,
                               double volume,
                               BuySell buySell)
    {
        set<Date> averagingDates = getAllBusinessDaysBetween(startOfAveraging, endOfAveraging, calendar);
        constructorCommon(averagingDates, settlement, strike, volume, buySell);
    }

    AsianForward::AsianForward(AsianForward& forward)
    {
        vector<Date> tmpDatesVector = forward.getAveragingDates();
        set<Date> dates = set<Date>(tmpDatesVector.begin(), tmpDatesVector.end());
        constructorCommon(dates, forward.settlementDate, forward.strike, forward.volume, forward.buySell);
    }

    std::unique_ptr<Contract> AsianForward::clone()
    {
        return std::make_unique<AsianForward>(*this);
    }

    void AsianForward::recordAsianIssue(std::string_view code,
                                        std::string_view message,
                                        const std::vector<std::string>& path)
    {
        recordIssue(code, message, path);
    }

    void AsianForward::absorbAsianReport(const ValidationReport& child,
                                         const std::vector<std::string>& prefix)
    {
        absorbChildReport(child, prefix);
    }

    void AsianForward::runChecks()
    {
        ForwardContract::runChecks();
        AsianContract::checkAveraging();
        if (!orderedAveragingDates.empty() && orderedAveragingDates.back() != getMaturityDate())
        {
            recordIssue(issues::ASIAN_MATURITY_NOT_FINAL_AVERAGE,
                        "Maturity date does not equal the final averaging date", {"maturity_date"});
        }
        if (!orderedAveragingDates.empty() && orderedAveragingDates.back() > getSettlementDate())
        {
            recordIssue(issues::ASIAN_SETTLEMENT_BEFORE_FINAL_AVERAGE,
                        "Settlement date before the final averaging date", {"settlement_date"});
        }
    }

    void AsianForward::runChecks(RatesEnvironment& re)
    {
        ForwardContract::runChecks(re);
        AsianContract::checkAveraging(re);
    }

    void AsianForward::setAveragingDates(set<Date> averagingDatesInput)
    {
        AsianContract::setAveragingDates(averagingDatesInput);
        setMaturityDate(orderedAveragingDates.back());
    }

    double AsianForward::getValue(std::shared_ptr<RatesEnvironment> re)
    {
        Date valueDate = re->getAnchorDate();
        if (valueDate > getSettlementDate())
        {
            return 0;
        }
        SJD_ASSERT(ForwardContract::isOK());

        double premium = 0;
        vector<double> forwards, fixes;
        getFixingAndForwardRates(valueDate, fixes, forwards, re);
        double df = (re->getDRS())->getDiscountFactor(getSettlementDate());

        double averageForward = 0;
        for (size_t i = 0; i < forwards.size(); ++i)
        {
            averageForward += forwards[i];
        }
        double averageRateSet = 0;
        for (size_t i = 0; i < fixes.size(); ++i)
        {
            averageRateSet += fixes[i];
        }

        if (orderedAveragingDates.size() > 0)
        {
            averageForward /= orderedAveragingDates.size();
            averageRateSet /= orderedAveragingDates.size();
        }

        if (valueDate < getMaturityDate())
        {
            premium = (averageForward - (strike - averageRateSet)) * df;
            return premium * volume * sign(buySell);
        }
        else if ((valueDate >= maturityDate) && (valueDate <= settlementDate))
        {
            [[maybe_unused]] double averageFwd = 0;
            premium = (averageRateSet - strike) * df;
            return premium * volume * sign(buySell);
        }
        return 0;
    }

    void AsianForward::getFixingAndForwardRates(Date valueDate,
                                                vector<double>& fixingRates,
                                                vector<double>& forwardRates,
                                                std::shared_ptr<RatesEnvironment> re)
    {
        fixingRates.clear();
        forwardRates.clear();
        vector<Date> fixingDates, forwardDates;
        AsianContract::getFixingAndForwardDates(valueDate, fixingDates, forwardDates);

        for (size_t i = 0; i < fixingDates.size(); ++i)
        {
            fixingRates.push_back(re->getFixingRS()->getRateSetOn(fixingDates[i]));
        }
        for (size_t i = 0; i < forwardDates.size(); ++i)
        {
            forwardRates.push_back((re->getFRS())->getForward(forwardDates[i]));
        }

        return;
    }

    string AsianForward::toStringRates(std::shared_ptr<RatesEnvironment> re)
    {
        stringstream descriptionStream;
        descriptionStream << SingleInstrumentContract::toStringRates(re);

        Contract::validate();
        if (ForwardContract::isOK())
        {
            descriptionStream << ",Fwd(Set)," << fixed << setprecision(4)
                              << (re->getFRS())->getForward(getSettlementDate());

            vector<double> forwards, fixes;
            getFixingAndForwardRates((re->getFRS())->getAnchorDate(), fixes, forwards, re);
            double aveFwd = 0;
            for (size_t i = 0; i < forwards.size(); ++i)
            {
                aveFwd += forwards[i];
            }
            if (forwards.size() > 0)
                aveFwd /= forwards.size();

            double aveRateSet = 0;
            for (size_t i = 0; i < fixes.size(); ++i)
            {
                aveRateSet += fixes[i];
            }
            if (fixes.size() > 0)
            {
                aveRateSet /= fixes.size();
            }

            descriptionStream << ",Fix(Ave)," << fixed << setprecision(4) << aveRateSet;
            descriptionStream << ",Fwd(Ave)," << fixed << setprecision(4) << aveFwd;
        }
        return descriptionStream.str();
    }

}
