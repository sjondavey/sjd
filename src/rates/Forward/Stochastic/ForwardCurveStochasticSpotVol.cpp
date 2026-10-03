#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticSpotVol.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    ForwardCurveStochasticSpotVol::ForwardCurveStochasticSpotVol(Date anchorDateInput,
                                                                 vector<Date> observationDatesInput,
                                                                 vector<double> forwardsInput,
                                                                 ArrayInterpolatorType typeInput,
                                                                 bool allowExtrapolationInput,
                                                                 boost::shared_ptr<VolatilityRateSource> vrs)
    {
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("ForwardCurveStochasticSpotVol"));
        setParameters(anchorDateInput, observationDatesInput, forwardsInput, typeInput, allowExtrapolationInput, vrs);
    }

    void ForwardCurveStochasticSpotVol::setParameters(Date anchorDateInput,
                                                      vector<Date> observationDatesInput,
                                                      vector<double> forwardsInput,
                                                      ArrayInterpolatorType typeInput,
                                                      bool allowExtrapolationInput,
                                                      boost::shared_ptr<VolatilityRateSource> vrs)
    {
        quotes.setInterpolated(anchorDateInput,
                               observationDatesInput,
                               forwardsInput,
                               typeInput,
                               allowExtrapolationInput,
                               ForwardAveraging::Point,
                               ForwardRoll::SpotVolatility,
                               vrs);
        anchorDate = quotes.anchorDate();
        allowExtrapolation = quotes.allowsExtrapolation();
        finalDate = quotes.finalDate();
        setVolitalityRateSource(vrs);
    }

    ForwardCurveStochasticSpotVol::~ForwardCurveStochasticSpotVol() {}

    bool ForwardCurveStochasticSpotVol::isOK()
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
        Date d1, d2;
        if (vrs->hasNegativeATMFwdFwdVariances(d1, d2))
        {
            stringstream errorDescription;
            errorDescription << "Negative fwd fwd variance between " << QuantLib::io::iso_date(d1)
                             << " and " << QuantLib::io::iso_date(d2);
            recordIssue("FORWARD_NEGATIVE_FWD_FWD_VARIANCE", errorDescription.str(), {"volatility_rate_source"});
        }
        return !errorTracking->getHasErrors();
    }

    double ForwardCurveStochasticSpotVol::getForward(Date toDate) const
    {
        return quotes.getForward(toDate);
    }

    boost::shared_ptr<ForwardRateSource> ForwardCurveStochasticSpotVol::rollForward(Date toDate, double normalRV)
    {
        return quotes.rollForward(toDate, normalRV, isInRange(toDate));
    }

    boost::shared_ptr<ForwardRateSource> ForwardCurveStochasticSpotVol::rollForward(Date toDate,
                                                                                    double normalRV,
                                                                                    vector<Date> rateSetDates,
                                                                                    vector<double>& rateSetsFromAnchorToDate)
    {
        return ForwardRateSourceStochastic::rollForward(toDate, normalRV, rateSetDates, rateSetsFromAnchorToDate);
    }

    void ForwardCurveStochasticSpotVol::setForwardValuationDates(set<Date> datesInput)
    {
        ForwardRateSourceStochastic::setForwardValuationDates(datesInput);
        if (forwardSimulationDates[0] < anchorDate || forwardSimulationDates.back() > finalDate)
        {
            forwardSimulationDates = vector<Date>();
            return;
        }
        quotes.preparePath(forwardSimulationDates, vector<Date>());
    }

    vector<boost::shared_ptr<ForwardRateSource>> ForwardCurveStochasticSpotVol::rollForward()
    {
        return quotes.rollPath(normalRVs, forwardSimulationDates);
    }

    vector<boost::shared_ptr<ForwardRateSource>> ForwardCurveStochasticSpotVol::rollForward(
        vector<Date> rateSetDates,
        vector<double>& rateSetsFromAnchorToFinalRollDate)
    {
        return ForwardRateSourceStochastic::rollForward(rateSetDates, rateSetsFromAnchorToFinalRollDate);
    }

}
