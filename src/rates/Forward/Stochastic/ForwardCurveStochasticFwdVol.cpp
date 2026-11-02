#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFwdVol.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    ForwardCurveStochasticFwdVol::ForwardCurveStochasticFwdVol(Date anchorDateInput,
                                                               vector<Date> observationDatesInput,
                                                               vector<double> forwardsInput,
                                                               ArrayInterpolatorType typeInput,
                                                               bool allowExtrapolationInput,
                                                               std::shared_ptr<VolatilityRateSource> vrs)
    {
        errorTracking =
            std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("ForwardCurveStochasticFwdVol"));
        setParameters(anchorDateInput, observationDatesInput, forwardsInput, typeInput,
                      allowExtrapolationInput, vrs);
    }

    void ForwardCurveStochasticFwdVol::setParameters(Date anchorDateInput,
                                                     vector<Date> observationDatesInput,
                                                     vector<double> forwardsInput,
                                                     ArrayInterpolatorType typeInput,
                                                     bool allowExtrapolationInput,
                                                     std::shared_ptr<VolatilityRateSource> volatility)
    {
        quotes.setInterpolated(anchorDateInput, observationDatesInput, forwardsInput, typeInput,
                               allowExtrapolationInput, ForwardAveraging::Point,
                               ForwardRoll::ForwardVolatility, volatility);
        anchorDate = quotes.anchorDate();
        allowExtrapolation = quotes.allowsExtrapolation();
        finalDate = quotes.finalDate();
        setVolatilityRateSource(volatility);
    }

    ForwardCurveStochasticFwdVol::~ForwardCurveStochasticFwdVol() {}

    bool ForwardCurveStochasticFwdVol::isOK()
    {
        if (!ForwardRateSourceStochastic::isOK())
        {
            return false;
        }
        string errorMessage = "";
        if (!quotes.quotesAreOk(errorMessage))
        {
            recordIssue("FORWARD_CURVE_INVALID", errorMessage, {"forward_curve"});
            return false;
        }
        return !errorTracking->getHasErrors();
    }

    double ForwardCurveStochasticFwdVol::getForward(Date toDate) const
    {
        return quotes.getForward(toDate);
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveStochasticFwdVol::rollForward(Date toDate, double normalRV)
    {
        return quotes.rollForward(toDate, normalRV, isInRange(toDate));
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveStochasticFwdVol::rollForward(
        Date toDate, double normalRV, vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToDate)
    {
        return ForwardRateSourceStochastic::rollForward(toDate, normalRV, rateSetDates,
                                                        rateSetsFromAnchorToDate);
    }

    void ForwardCurveStochasticFwdVol::setForwardValuationDates(set<Date> datesInput)
    {
        ForwardRateSourceStochastic::setForwardValuationDates(datesInput);
        if (forwardSimulationDates[0] < anchorDate || forwardSimulationDates.back() > finalDate)
        {
            forwardSimulationDates = vector<Date>();
            return;
        }
        quotes.preparePath(forwardSimulationDates, vector<Date>());
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurveStochasticFwdVol::rollForward()
    {
        return quotes.rollPath(normalRVs, forwardSimulationDates);
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurveStochasticFwdVol::rollForward(
        vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToFinalRollDate)
    {
        return ForwardRateSourceStochastic::rollForward(rateSetDates, rateSetsFromAnchorToFinalRollDate);
    }

}
