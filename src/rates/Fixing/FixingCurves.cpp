#include <sjd/rates/Fixing/FixingCurves.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    DailyFixingCurve
     =======================================================================================*/
    DailyFixingCurve::DailyFixingCurve(vector<Date> fixingDatesInput, vector<double> fixingRatesInput)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("DailyFixingCurve"));
        setDatesAndRates(fixingDatesInput, fixingRatesInput);
        captureOriginalState();
    }

    std::shared_ptr<RollingFixingRateSource> DailyFixingCurve::rollForward(Date toDate,
                                                                           vector<Date> newRateSetDates,
                                                                           vector<double> newRateSets)
    {
        std::shared_ptr<RollingFixingRateSource> rolledFixingRS =
            std::shared_ptr<RollingFixingRateSource>(new DailyFixingCurve(fixingDates, fixingRates));
        rolledFixingRS->appendRateSets(newRateSetDates, newRateSets);
        return rolledFixingRS;
    }

    /*======================================================================================
    MonthlyFixingCurve
    =======================================================================================*/
    MonthlyFixingCurve::MonthlyFixingCurve(vector<Date> fixingDatesInput, vector<double> fixingRatesInput)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("MonthlyFixingCurve"));

        vector<Date> monthDates = vector<Date>(0);
        for (size_t i = 0; i < min(fixingDatesInput.size(), fixingRatesInput.size()); ++i)
        {
            monthDates.push_back(Date(1, fixingDatesInput[i].month(), fixingDatesInput[i].year()));
        }
        setDatesAndRates(monthDates, fixingRatesInput);
        captureOriginalState();
    }

    MonthlyFixingCurve::~MonthlyFixingCurve() {}

    std::shared_ptr<RollingFixingRateSource> MonthlyFixingCurve::rollForward(Date toDate,
                                                                             vector<Date> newRateSetDates,
                                                                             vector<double> newRateSets)
    {
        std::shared_ptr<RollingFixingRateSource> rolledFixingRS =
            std::shared_ptr<RollingFixingRateSource>(new MonthlyFixingCurve(fixingDates, fixingRates));
        rolledFixingRS->appendRateSets(newRateSetDates, newRateSets);
        return rolledFixingRS;
    }

    vector<Date> MonthlyFixingCurve::getFixingDatesFor(vector<Date> inputDates)
    {
        vector<Date> dateSubset;
        Date d1 = Date(1, inputDates[0].month(), inputDates[0].year());
        Date previousMonth = d1 - Period(1, Months);
        if (previousMonth > originalDates.back())
        {
            dateSubset.push_back(previousMonth);
        }
        if (d1 > originalDates.back())
        {
            dateSubset.push_back(d1);
        }
        Date d2;
        for (size_t i = 1; i < inputDates.size(); ++i)
        {
            d2 = Date(1, inputDates[i].month(), inputDates[i].year());
            previousMonth = d2 - Period(1, Months);

            if (previousMonth > d1)
            {
                if (previousMonth > originalDates.back())
                {
                    dateSubset.push_back(previousMonth);
                }
            }
            if (d2 > d1)
            {
                if (d2 > originalDates.back())
                {
                    dateSubset.push_back(d2);
                }
                d1 = d2;
            }
        }
        return dateSubset;
    }

    void MonthlyFixingCurve::appendRateSets(vector<Date> appendedDates, vector<double> appendedRates)
    {
        vector<Date> monthDates = vector<Date>(0);
        for (size_t i = 0; i < appendedDates.size(); ++i)
        {
            monthDates.push_back(Date(1, appendedDates[i].month(), appendedDates[i].year()));
        }
        RollingFixingRateSource::appendRateSets(monthDates, appendedRates);
    }

    bool MonthlyFixingCurve::containsARateSetOn(QuantLib::Date date) const
    {
        return FixingRateSource::containsARateSetOn(Date(1, date.month(), date.year()));
    }

    double MonthlyFixingCurve::getRateSetOn(QuantLib::Date date) const
    {
        return FixingRateSource::getRateSetOn(Date(1, date.month(), date.year()));
    }

    vector<double> MonthlyFixingCurve::getRateSetOn(vector<Date> toDates) const
    {
        if (toDates.size() == 0)
        {
            return vector<double>(0);
        }
        vector<double> rates = vector<double>(toDates.size());
        Month month = toDates[0].month();
        Year year = toDates[0].year();
        double rateSet = FixingRateSource::getRateSetOn(Date(1, month, year));
        for (size_t i = 0; i < toDates.size(); ++i)
        {
            Month newMonth = toDates[i].month();
            Year newYear = toDates[i].year();
            if (newMonth == month && newYear == year)
            {
                rates[i] = rateSet;
            }
            else
            {
                month = newMonth;
                year = newYear;
                rateSet = FixingRateSource::getRateSetOn(Date(1, month, year));
                rates[i] = rateSet;
            }
        }
        return rates;
    }

    /*======================================================================================
    HistoricFedFundsCurve
    =======================================================================================*/
    HistoricFedFundsCurve::HistoricFedFundsCurve(vector<Date> fixingDatesInput,
                                                 vector<double> fixingRatesInput)
        : DailyFixingCurve(fixingDatesInput, fixingRatesInput)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("HistoricFedFundsCurve"));
    }

    HistoricFedFundsCurve::~HistoricFedFundsCurve() {}

    bool HistoricFedFundsCurve::isOK()
    {
        DailyFixingCurve::isOK();

        for (size_t i = 0; i < fixingRates.size(); ++i)
        {
            if ((fixingRates[i] < 0) || (fixingRates[i] > 1))
            {
                recordIssue("FIXING_RATE_OUT_OF_RANGE",
                            std::to_string(i) + "-th interest rate outside the range [0,1]",
                            {"fixing_rates"});
            }
        }
        return !errorTracking->getHasErrors();
    }

    bool HistoricFedFundsCurve::canCalculateInterestFor(Date startDate,
                                                        Date endDate,
                                                        vector<Date> marginCallDates,
                                                        vector<double> marginCallAmounts)
    {
        if (!isOK())
        {
            return false;
        }
        if (marginCallDates.size() != marginCallAmounts.size())
        {
            return false;
        }
        if (!isStrictlyIncreasing<Date>(marginCallDates))
        {
            return false;
        }
        if ((marginCallDates.front() < startDate) || (marginCallDates.back() > endDate))
        {
            return false;
        }
        return true;
    }

    double HistoricFedFundsCurve::calculateNonCompoundedInterestOnMargin(double openingBalance,
                                                                         Date startDate,
                                                                         Date endDate,
                                                                         vector<Date> marginCallDates,
                                                                         vector<double> marginCallAmounts)
    {
        if (!canCalculateInterestFor(startDate, endDate, marginCallDates, marginCallAmounts))
        {
            return 0;
        }
        Date calcuationDate = startDate;
        size_t marginCallIndex = 0;
        double balance = openingBalance;
        double cumulativeInterest = 0;
        while (calcuationDate <= endDate)
        {
            if (calcuationDate == marginCallDates[marginCallIndex])
            {
                balance += marginCallAmounts[marginCallIndex];
                ++marginCallIndex;
            }
            double rate = getRateSetOn(calcuationDate);
            cumulativeInterest += boost::math::round((rate / 360.0) * balance * 100) / 100;
            calcuationDate += 1;
        }
        return cumulativeInterest;
    }

}