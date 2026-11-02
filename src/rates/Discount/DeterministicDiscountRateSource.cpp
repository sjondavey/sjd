#include <sjd/rates/Discount/DeterministicDiscountRateSource.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    bool DeterministicDiscountRateSource::isOK()
    {
        if (!RateSource::isOK())
        {
            return false;
        }
        if (orderedForwardDates.size() > 0)
        {
            if (!isInRange(orderedForwardDates.front()))
            {
                recordIssue("DISCOUNT_RATE_SOURCE_RANGE",
                            "First forward date is outside the rate source range", {"observation_dates"});
                return false;
            }
            if (!isInRange(orderedForwardDates.back()))
            {
                recordIssue("DISCOUNT_RATE_SOURCE_RANGE",
                            "Last forward date is outside the rate source range", {"observation_dates"});
                return false;
            }
        }
        return true;
    }

    vector<pair<string, std::shared_ptr<RateSource>>> DeterministicDiscountRateSource::getMarketRateStresses()
    {
        std::shared_ptr<QuantLib::DayCounter> dayCounter = std::make_shared<Actual360>();
        std::shared_ptr<InterestRateConvention> irc =
            std::make_shared<InterestRateConvention>(dayCounter, Compounded, Annual);
        vector<pair<string, std::shared_ptr<RateSource>>> marketStress;
        marketStress.push_back(make_pair("DF + 1% NACA a/360", parallelBump(0.01, irc)));
        return marketStress;
    }

    vector<std::shared_ptr<DiscountRateSource>> DeterministicDiscountRateSource::rollForward()
    {
        vector<std::shared_ptr<DiscountRateSource>> discountRateSources;
        for (size_t i = 0; i < orderedForwardDates.size(); ++i)
        {
            discountRateSources.push_back(rollForward(orderedForwardDates[i]));
        }
        return discountRateSources;
    }

    void DeterministicDiscountRateSource::setForwardValuationDates(set<Date> dates)
    {
        orderedForwardDates = vector<Date>(dates.begin(), dates.end());
    }
}