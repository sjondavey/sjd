#include <sjd/rates/Volatility/DeterministicVolatilityRateSource.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{
    bool DeterministicVolatilityRateSource::isOK()
    {
        if (!VolatilityRateSource::isOK())
        {
            return false;
        }
        if (orderedForwardDates.size() > 0)
        {
            if (!isInRange(orderedForwardDates.front()))
            {
                recordIssue("VOLATILITY_RATE_SOURCE_RANGE",
                            "First forward date is outside the rate source range", {"observation_dates"});
                return false;
            }
            if (!isInRange(orderedForwardDates.back()))
            {
                recordIssue("VOLATILITY_RATE_SOURCE_RANGE",
                            "Last forward date is outside the rate source range", {"observation_dates"});
                return false;
            }
        }
        return true;
    }

    vector<pair<string, std::shared_ptr<RateSource>>> DeterministicVolatilityRateSource::
        getMarketRateStresses()
    {
        vector<pair<string, std::shared_ptr<RateSource>>> marketStress;
        marketStress.push_back(make_pair("Vol + 1% ", parallelBump(0.01)));
        return marketStress;
    }

    vector<std::shared_ptr<VolatilityRateSource>> DeterministicVolatilityRateSource::rollForward()
    {
        vector<std::shared_ptr<VolatilityRateSource>> volatilityRateSources;
        for (size_t i = 0; i < orderedForwardDates.size(); ++i)
        {
            volatilityRateSources.push_back(rollForward(orderedForwardDates[i]));
        }
        return volatilityRateSources;
    }

    void DeterministicVolatilityRateSource::setForwardValuationDates(set<Date> dates)
    {
        orderedForwardDates = vector<Date>(dates.begin(), dates.end());
    }
}