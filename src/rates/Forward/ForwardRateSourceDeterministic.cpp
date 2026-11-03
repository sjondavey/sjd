#include <sjd/rates/Forward/ForwardRateSourceDeterministic.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    void ForwardRateSourceDeterministic::runChecks()
    {
        RateSource::runChecks();
        if (hasErrors())
        {
            return;
        }
        if (orderedForwardDates.size() > 0)
        {
            if (!isInRange(orderedForwardDates.front()))
            {
                recordIssue(issues::FORWARD_RATE_SOURCE_RANGE,
                            "First forward date is outside the rate source range", {"observation_dates"});
                return;
            }
            if (!isInRange(orderedForwardDates.back()))
            {
                recordIssue(issues::FORWARD_RATE_SOURCE_RANGE,
                            "Last forward date is outside the rate source range", {"observation_dates"});
                return;
            }
        }
        return;
    }

    vector<pair<string, std::shared_ptr<RateSource>>> ForwardRateSourceDeterministic::getMarketRateStresses()
    {
        vector<pair<string, std::shared_ptr<RateSource>>> marketStress;
        marketStress.push_back(make_pair("Fwd + 1unit", parallelBump(1.0)));
        return marketStress;
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardRateSourceDeterministic::rollForward()
    {
        vector<std::shared_ptr<ForwardRateSource>> frss;
        for (size_t i = 0; i < orderedForwardDates.size(); ++i)
        {
            frss.push_back(rollForward(orderedForwardDates[i]));
        }
        return frss;
    }

    std::shared_ptr<ForwardRateSource> ForwardRateSourceDeterministic::rollForward(
        Date toDate, vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToDate)
    {
        rateSetsFromAnchorToDate.clear();
        for (size_t i = 0; i < rateSetDates.size(); ++i)
        {
            rateSetsFromAnchorToDate.push_back(getForward(rateSetDates[i]));
        }
        return rollForward(toDate);
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardRateSourceDeterministic::rollForward(
        vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToFinalRollDate)
    {
        rateSetsFromAnchorToFinalRollDate.clear();
        vector<std::shared_ptr<ForwardRateSource>> frss;
        vector<double> previousRateSets;
        vector<double> rateSets;
        for (size_t i = 0; i < orderedForwardDates.size(); ++i)
        {
            vector<Date>::iterator it =
                lower_bound(rateSetDates.begin(), rateSetDates.end(), orderedForwardDates[i]);
            vector<Date> dateSubset = vector<Date>(rateSetDates.begin(), it);
            frss.push_back(rollForward(orderedForwardDates[i], dateSubset, rateSets));
            rateSetsFromAnchorToFinalRollDate.insert(rateSetsFromAnchorToFinalRollDate.end(),
                                                     rateSets.begin() + previousRateSets.size(),
                                                     rateSets.end());
            previousRateSets = rateSets;
        }
        return frss;
    }

    void ForwardRateSourceDeterministic::setForwardValuationDates(set<Date> dates)
    {
        orderedForwardDates = vector<Date>(dates.begin(), dates.end());
    }
}