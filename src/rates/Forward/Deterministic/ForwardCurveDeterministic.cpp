#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    ForwardCurveDeterministic::ForwardCurveDeterministic(Date anchorDateInput,
                                                         vector<Date> observationDatesInput,
                                                         vector<double> forwardsInput,
                                                         ArrayInterpolatorType typeInput,
                                                         bool allowExtrapolationInput)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("ForwardCurveDeterministic");
        setParameters(anchorDateInput, observationDatesInput, forwardsInput, typeInput,
                      allowExtrapolationInput);
    }

    void ForwardCurveDeterministic::setParameters(Date anchorDateInput,
                                                  vector<Date> observationDatesInput,
                                                  vector<double> forwardsInput,
                                                  ArrayInterpolatorType typeInput,
                                                  bool allowExtrapolationInput)
    {
        quotes.setInterpolated(anchorDateInput, observationDatesInput, forwardsInput, typeInput,
                               allowExtrapolationInput, ForwardAveraging::Point, ForwardRoll::Deterministic,
                               std::shared_ptr<VolatilityRateSource>());
        anchorDate = quotes.anchorDate();
        allowExtrapolation = quotes.allowsExtrapolation();
        flatBackFilled = quotes.anchorWasFlatFilled();
        finalDate = quotes.finalDate();
    }

    void ForwardCurveDeterministic::runChecks()
    {
        ForwardRateSourceDeterministic::runChecks();
        string errorMessage = "";
        if (!quotes.quotesAreOk(errorMessage))
        {
            const std::string_view code = errorMessage.find("same dimension") != std::string::npos
                                              ? issues::FORWARD_CURVE_DIMENSION_MISMATCH
                                              : issues::FORWARD_CURVE_INVALID;
            recordIssue(code, errorMessage, {"inputs"});
            return;
        }
        return;
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

    std::shared_ptr<ForwardRateSource> ForwardCurveDeterministic::parallelBump(double spread)
    {
        return quotes.parallelBump(spread);
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveDeterministic::rollForward(Date toDate)
    {
        return quotes.rollForward(toDate, isInRange(toDate));
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveDeterministic::rollForward(
        Date toDate, vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToDate)
    {
        return ForwardRateSourceDeterministic::rollForward(toDate, rateSetDates, rateSetsFromAnchorToDate);
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurveDeterministic::rollForward()
    {
        return ForwardRateSourceDeterministic::rollForward();
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurveDeterministic::rollForward(
        vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToFinalRollDate)
    {
        return ForwardRateSourceDeterministic::rollForward(rateSetDates, rateSetsFromAnchorToFinalRollDate);
    }

}
