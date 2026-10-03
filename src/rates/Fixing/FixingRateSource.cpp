#include <sjd/rates/Fixing/FixingRateSource.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{

    void FixingRateSource::setDatesAndRates(vector<Date> fixingDatesInput, vector<double> fixingRatesInput)
    {
        fixingDates = fixingDatesInput;
        fixingRates = fixingRatesInput;
        if (fixingDates.size() != 0) 
        {
            firstDate = fixingDates.front();
            lastDate =  fixingDates.back();
        }
    }

    std::string FixingRateSource::validationRoot() const
    {
        return "fixing_rate_source";
    }

    bool FixingRateSource::isOK()
    {
        errorTracking->clear();
        if (fixingDates.size() != fixingRates.size())
        {
            recordIssue("FIXING_DATES_AND_RATES_LENGTH",
                        "Number of Dates and Rates inputs are not equal",
                        {"fixing_dates"});
        }
        if (fixingDates.size() == 0) 
        {
            recordIssue("FIXING_DATES_EMPTY",
                        "No fixing dates entered",
                        {"fixing_dates"});
        }
        else 
        {
            if (!isStrictlyIncreasing<Date>(fixingDates))
            {
                recordIssue("FIXING_DATES_NOT_INCREASING",
                            "Input Dates are not strictly increasing",
                            {"fixing_dates"});
            }
            if (firstDate != fixingDates.front())
            {
                recordIssue("FIXING_FIRST_DATE_MISMATCH",
                            "First Fixing Date does not match the first date in the fixing vector",
                            {"fixing_dates"});
            }
        }
        return !errorTracking->getHasErrors();
    }

    bool FixingRateSource::containsARateSetOn(Date date) const
    {
        vector<Date>::const_iterator low;
        low = std::lower_bound (fixingDates.begin(), fixingDates.end(), date);
        if (low == fixingDates.cend())
        {
            return false;
        }
        return *low == date;
    }

    double FixingRateSource::getRateSetOn(Date date) const
    {
        vector<Date>::const_iterator low;
        low = std::lower_bound (fixingDates.begin(), fixingDates.end(), date);
        return fixingRates[low - fixingDates.begin()];
    }

    Date FixingRateSource::getEarliestDate() const 
    {
        return firstDate;
    }

    Date FixingRateSource::getMostRecentDate() const
    {
        return lastDate;
    }

    vector<double> FixingRateSource::getRateSetOn(vector<Date> toDates) const
    {
        vector<double> rates;
        for (size_t i = 0; i < toDates.size(); ++i)
        {
            rates.push_back(getRateSetOn(toDates[i]));
        }
        return rates;
    }

}
