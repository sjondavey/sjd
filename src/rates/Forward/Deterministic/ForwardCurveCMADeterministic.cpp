#include <sjd/rates/Forward/Deterministic/ForwardCurveCMADeterministic.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    ForwardCurveCMADeterministic::ForwardCurveCMADeterministic(Date anchorDateInput,
                                                               vector<Date> observationDatesInput,
                                                               vector<double> forwardsInput,
                                                               ArrayInterpolatorType typeInput,
                                                               bool allowExtrapolationInput)
    {
        errorTracking =
            std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("ForwardCurveCMADeterministic"));
        quotes.setInterpolated(anchorDateInput, observationDatesInput, forwardsInput, typeInput,
                               allowExtrapolationInput, ForwardAveraging::CalendarMonth,
                               ForwardRoll::Deterministic, std::shared_ptr<VolatilityRateSource>());
        anchorDate = quotes.anchorDate();
        allowExtrapolation = quotes.allowsExtrapolation();
        finalDate = quotes.finalDate();
    }

    bool ForwardCurveCMADeterministic::isOK()
    {
        if (!ForwardRateSourceDeterministic::isOK())
        {
            return false;
        }
        string errorMessage = "";
        if (!quotes.quotesAreOk(errorMessage))
        {
            const std::string code = errorMessage.find("same dimension") != std::string::npos
                                         ? "FORWARD_CURVE_CMA_DIMENSION_MISMATCH"
                                         : "FORWARD_CURVE_CMA_INVALID";
            recordIssue(code, errorMessage, {"inputs"});
            return false;
        }
        return !errorTracking->getHasErrors();
    }

    bool ForwardCurveCMADeterministic::isInRange(Date d) const
    {
        return quotes.calendarMonthInRange(d);
    }

    bool ForwardCurveCMADeterministic::extendsTo(Date d) const
    {
        Date beginningOfMonth = Date(1, d.month(), d.year());
        return RateSource::extendsTo(beginningOfMonth);
    }

    vector<Date> ForwardCurveCMADeterministic::getObservationDates()
    {
        return quotes.observationDates();
    }

    double ForwardCurveCMADeterministic::getForward(Date toDate) const
    {
        return quotes.getForward(toDate);
    }

    vector<double> ForwardCurveCMADeterministic::getForward(vector<QuantLib::Date> toDates) const
    {
        return quotes.getForward(toDates);
    }

    double ForwardCurveCMADeterministic::getForward(Month month, Year year) const
    {
        return quotes.getForward(month, year);
    }

    void ForwardCurveCMADeterministic::setForwards(vector<double> updatedForwards)
    {
        quotes.setForwards(updatedForwards);
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveCMADeterministic::rollForward(Date toDate)
    {
        return quotes.rollForward(toDate, isInRange(toDate));
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveCMADeterministic::rollForward(
        Date toDate, vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToDate)
    {
        return ForwardRateSourceDeterministic::rollForward(toDate, rateSetDates, rateSetsFromAnchorToDate);
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveCMADeterministic::parallelBump(double spread)
    {
        return quotes.parallelBump(spread);
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurveCMADeterministic::rollForward()
    {
        return ForwardRateSourceDeterministic::rollForward();
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurveCMADeterministic::rollForward(
        vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToFinalRollDate)
    {
        return ForwardRateSourceDeterministic::rollForward(rateSetDates, rateSetsFromAnchorToFinalRollDate);
    }

}
