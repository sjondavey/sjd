#include <sjd/rates/Fixing/RollingFixingRateSource.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    void RollingFixingRateSource::captureOriginalState()
    {
        originalDates = fixingDates;
        originalFixes = fixingRates;
        originalFinalDate = lastDate;
    }

    vector<Date> RollingFixingRateSource::getFixingDatesFor(vector<Date> inputDates)
    {
        return inputDates;
    }

    void RollingFixingRateSource::updateAppedededRates(vector<double> newFixingRates)
    {
        size_t numberOfOriginalFixes = originalFixes.size();
        for (size_t i = 0; i < newFixingRates.size(); ++i)
        {
            fixingRates[numberOfOriginalFixes + i] = newFixingRates[i];
        }
    }

    void RollingFixingRateSource::reset()
    {
        fixingDates = originalDates;
        fixingRates = originalFixes;
        lastDate = originalFinalDate;
    }

    void RollingFixingRateSource::appendRateSets(vector<Date> appendedDates, vector<double> appendedRates)
    {
        fixingDates = originalDates;
        fixingDates.insert(fixingDates.end(), appendedDates.begin(), appendedDates.end());
        fixingRates = originalFixes;
        fixingRates.insert(fixingRates.end(), appendedRates.begin(), appendedRates.end());
        lastDate = fixingDates.back();
    }
}