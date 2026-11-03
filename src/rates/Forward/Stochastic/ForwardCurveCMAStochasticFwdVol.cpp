#include <sjd/rates/Forward/Stochastic/ForwardCurveCMAStochasticFwdVol.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    ForwardCurveCMAStochasticFwdVol::ForwardCurveCMAStochasticFwdVol(
        Date anchorDateInput,
        vector<Date> observationDatesInput,
        vector<double> forwardsInput,
        ArrayInterpolatorType typeInput,
        bool allowExtrapolationInput,
        std::shared_ptr<VolatilityRateSource> vrs)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("ForwardCurveCMAStochasticFwdVol");
        quotes.setInterpolated(anchorDateInput, observationDatesInput, forwardsInput, typeInput,
                               allowExtrapolationInput, ForwardAveraging::CalendarMonth,
                               ForwardRoll::ForwardVolatility, vrs);
        anchorDate = quotes.anchorDate();
        allowExtrapolation = quotes.allowsExtrapolation();
        flatBackFilled = quotes.anchorWasFlatFilled();
        finalDate = quotes.finalDate();
        setVolatilityRateSource(vrs);
    }

    void ForwardCurveCMAStochasticFwdVol::runChecks()
    {
        ForwardRateSourceStochastic::runChecks();
        if (hasErrors())
        {
            return;
        }
        string errorMessage = "";
        if (!quotes.quotesAreOk(errorMessage))
        {
            recordIssue(issues::FORWARD_CURVE_INVALID, errorMessage, {"forward_curve"});
            return;
        }
        return;
    }

    bool ForwardCurveCMAStochasticFwdVol::isInRange(Date d) const
    {
        return quotes.calendarMonthInRange(d);
    }

    bool ForwardCurveCMAStochasticFwdVol::extendsTo(Date date) const
    {
        Date beginningOfMonth = Date(1, date.month(), date.year());
        return RateSource::extendsTo(beginningOfMonth);
    }

    vector<Date> ForwardCurveCMAStochasticFwdVol::getObservationDates()
    {
        return quotes.observationDates();
    }

    vector<double> ForwardCurveCMAStochasticFwdVol::getForwards()
    {
        return quotes.observationPoints();
    }

    void ForwardCurveCMAStochasticFwdVol::setForwards(vector<double> updatedForwards)
    {
        quotes.setForwards(updatedForwards);
    }

    double ForwardCurveCMAStochasticFwdVol::getForward(Date toDate) const
    {
        return quotes.getForward(toDate);
    }

    vector<double> ForwardCurveCMAStochasticFwdVol::getForward(vector<QuantLib::Date> toDates) const
    {
        return quotes.getForward(toDates);
    }

    double ForwardCurveCMAStochasticFwdVol::getForward(Month month, Year year) const
    {
        return quotes.getForward(month, year);
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveCMAStochasticFwdVol::rollForward(Date toDateInput,
                                                                                    double normalRV)
    {
        return quotes.rollForward(toDateInput, normalRV, isInRange(toDateInput));
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveCMAStochasticFwdVol::rollForward(
        Date toDate, double normalRV, vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToDate)
    {
        return ForwardRateSourceStochastic::rollForward(toDate, normalRV, rateSetDates,
                                                        rateSetsFromAnchorToDate);
    }

    void ForwardCurveCMAStochasticFwdVol::setForwardValuationDates(set<Date> datesInput)
    {
        vector<Date> startOfSimulationMonths;
        for (set<Date>::iterator it = datesInput.begin(); it != datesInput.end(); ++it)
        {
            startOfSimulationMonths.push_back(Date(1, it->month(), it->year()));
        }
        ForwardRateSourceStochastic::setForwardValuationDates(datesInput);
        if (forwardSimulationDates[0] < anchorDate || forwardSimulationDates.back() > finalDate)
        {
            forwardSimulationDates = vector<Date>();
            return;
        }
        quotes.preparePath(forwardSimulationDates, startOfSimulationMonths);
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurveCMAStochasticFwdVol::rollForward()
    {
        return quotes.rollPath(normalRVs, forwardSimulationDates);
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurveCMAStochasticFwdVol::rollForward(
        vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToFinalRollDate)
    {
        return ForwardRateSourceStochastic::rollForward(rateSetDates, rateSetsFromAnchorToFinalRollDate);
    }

    vector<double> ForwardCurveCMAStochasticFwdVol::getImpliedRandomNumbers(vector<double> spot)
    {
        return quotes.impliedRandomNumbers(spot, normalRVs, anchorDate, forwardSimulationDates);
    }

}
