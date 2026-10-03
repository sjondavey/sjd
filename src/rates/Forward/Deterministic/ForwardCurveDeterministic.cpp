#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    ForwardCurveDeterministic::ForwardCurveDeterministic(Date anchorDateInput,
                                                         vector<Date> observationDatesInput,
                                                         vector<double> forwardsInput,
                                                         ArrayInterpolatorType typeInput,
                                                         bool allowExtrapolationInput)
    {
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("ForwardCurveDeterministic"));
        setParameters(anchorDateInput, observationDatesInput, forwardsInput, typeInput, allowExtrapolationInput);
    }

    void ForwardCurveDeterministic::setParameters(Date anchorDateInput,
                                                  vector<Date> observationDatesInput,
                                                  vector<double> forwardsInput,
                                                  ArrayInterpolatorType typeInput,
                                                  bool allowExtrapolationInput)
    {
        quotes.setInterpolated(anchorDateInput,
                               observationDatesInput,
                               forwardsInput,
                               typeInput,
                               allowExtrapolationInput,
                               ForwardAveraging::Point,
                               ForwardRoll::Deterministic,
                               boost::shared_ptr<VolatilityRateSource>());
        anchorDate = quotes.anchorDate();
        allowExtrapolation = quotes.allowsExtrapolation();
        finalDate = quotes.finalDate();
    }

    bool ForwardCurveDeterministic::isOK()
    {
        ForwardRateSourceDeterministic::isOK();
        string errorMessage = "";
        if (!quotes.quotesAreOk(errorMessage))
        {
            const std::string code = errorMessage.find("same dimension") != std::string::npos
                ? "FORWARD_CURVE_DIMENSION_MISMATCH"
                : "FORWARD_CURVE_INVALID";
            recordIssue(code, errorMessage, {"inputs"});
            return false;
        }
        return !errorTracking->getHasErrors();
    }

    double ForwardCurveDeterministic::getForward(Date toDate) const
    {
        return quotes.getForward(toDate);
    }

    vector<Date> ForwardCurveDeterministic::getObservationDates()
    {
        return quotes.observationDates();
    }

    vector<double> ForwardCurveDeterministic::getForwards()
    {
        return quotes.rates();
    }

    void ForwardCurveDeterministic::setForwards(vector<double> updatedForwards)
    {
        quotes.setForwards(updatedForwards);
    }

    boost::shared_ptr<ForwardRateSource> ForwardCurveDeterministic::parallelBump(double spread)
    {
        return quotes.parallelBump(spread);
    }

    boost::shared_ptr<ForwardRateSource> ForwardCurveDeterministic::rollForward(Date toDate)
    {
        return quotes.rollForward(toDate, isInRange(toDate));
    }

    boost::shared_ptr<ForwardRateSource> ForwardCurveDeterministic::rollForward(Date toDate,
                                                                                vector<Date> rateSetDates,
                                                                                vector<double>& rateSetsFromAnchorToDate)
    {
        return ForwardRateSourceDeterministic::rollForward(toDate, rateSetDates, rateSetsFromAnchorToDate);
    }

    vector<boost::shared_ptr<ForwardRateSource>> ForwardCurveDeterministic::rollForward()
    {
        return ForwardRateSourceDeterministic::rollForward();
    }

    vector<boost::shared_ptr<ForwardRateSource>> ForwardCurveDeterministic::rollForward(
        vector<Date> rateSetDates,
        vector<double>& rateSetsFromAnchorToFinalRollDate)
    {
        return ForwardRateSourceDeterministic::rollForward(rateSetDates, rateSetsFromAnchorToFinalRollDate);
    }

}
