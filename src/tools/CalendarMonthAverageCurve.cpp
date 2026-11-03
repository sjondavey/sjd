#include <sjd/tools/CalendarMonthAverageCurve.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    /*======================================================================================
    CalendarMonthAverageCurve
    =======================================================================================*/
    CalendarMonthAverageCurve::CalendarMonthAverageCurve()
    {
        errorTracking = std::make_shared<IssueTracker>("CalendarMonthAverageCurve");
    }

    CalendarMonthAverageCurve::CalendarMonthAverageCurve(Date anchorDateInput,
                                                         vector<Date> observationDatesInput,
                                                         vector<double> observedRatesInput,
                                                         ArrayInterpolatorType typeInput,
                                                         bool allowExtrapolationInput)
    {
        errorTracking = std::make_shared<IssueTracker>("CalendarMonthAverageCurve");
        setParameters(anchorDateInput, observationDatesInput, observedRatesInput, typeInput,
                      allowExtrapolationInput);
    }

    void CalendarMonthAverageCurve::setParameters(Date anchorDateInput,
                                                  vector<Date> observationDatesInput,
                                                  vector<double> observedRatesInput,
                                                  ArrayInterpolatorType typeInput,
                                                  bool allowExtrapolationInput)
    {
        type = typeInput;
        observationDates = observationDatesInput;
        observedRates = observedRatesInput;
        dayCounter = std::make_shared<SimpleDayCounter>();
        a365DayCounter = std::make_shared<Actual365Fixed>();
        anchorDate = anchorDateInput;
        beginningOfAnchorDateMonth = Date(1, anchorDateInput.month(), anchorDateInput.year());
        allowExtrapolation = allowExtrapolationInput;

        if (observationDates.size() < 1)
        {
            finalDate = anchorDate;
            interpolator = std::make_shared<LinearArrayInterpolator>(
                vector<double>(1, 0), vector<double>(1, 1), allowExtrapolation);
            return;
        }
        finalDate = observationDates.back();

        // vector<double> monthsAhead = vector<double>(observationDatesInput.size());
        observationPoints = vector<double>(observationDatesInput.size());
        for (size_t i = 0; i < observationDatesInput.size(); ++i)
        {
            observationPoints[i] = getNumberOfMonthsAheadOfAnchorDate(observationDatesInput[i]);
        }

        switch (type)
        {
        case ArrayInterpolatorType::CUBIC:
            interpolator = std::make_shared<CubicSplineInterpolator>(observationPoints, observedRates,
                                                                     allowExtrapolation);
            break;
        default:
            interpolator = std::make_shared<LinearArrayInterpolator>(observationPoints, observedRates,
                                                                     allowExtrapolation);
        }
    }

    void CalendarMonthAverageCurve::runChecks()
    {
        if (observationPoints.size() < 1)
        {
            recordIssue(issues::CURVE_INPUT_EMPTY, "Input time empty", {});
            return;
        }
        if (observationPoints[0] < 0)
        {
            recordIssue(issues::CURVE_OBSERVATION_BEFORE_ANCHOR, "First observation time before Anchor Date",
                        {});
            return;
        }
        if (interpolator && !interpolator->isOk())
            absorbChildReport(interpolator->validationReport(), {});
    }

    Date CalendarMonthAverageCurve::getAnchorDate() const
    {
        return anchorDate;
    }

    Date CalendarMonthAverageCurve::getBeginningOfAnchorDateMonth()
    {
        return beginningOfAnchorDateMonth;
    }

    Date CalendarMonthAverageCurve::getFinalDate()
    {
        return finalDate;
    }

    ArrayInterpolatorType CalendarMonthAverageCurve::getType()
    {
        return type;
    }

    bool CalendarMonthAverageCurve::getAllowsExtrapolation()
    {
        return allowExtrapolation;
    }

    vector<double> CalendarMonthAverageCurve::getRates()
    {
        return observedRates;
    }

    void CalendarMonthAverageCurve::setRates(vector<double> updatedRates)
    {
        observedRates = updatedRates;
        interpolator->setYVector(updatedRates);
    }

    bool CalendarMonthAverageCurve::containsRateOnAnchorDate()
    {
        Date d1 = Date(1, observationDates[0].month(), observationDates[0].year());
        return (d1 == beginningOfAnchorDateMonth);
    }

    void CalendarMonthAverageCurve::addPointOnAnchorDate(double valueToAdd)
    {
        vector<Date> observationPointsInput = observationDates;
        vector<double> observedRatesInput = observedRates;
        observationPointsInput.insert(observationPointsInput.begin(), anchorDate);
        observedRatesInput.insert(observedRatesInput.begin(), valueToAdd);

        setParameters(anchorDate, observationPointsInput, observedRatesInput, type, allowExtrapolation);
    }

    double CalendarMonthAverageCurve::getRate(Date toDate) const
    {
        int yf = getNumberOfMonthsAheadOfAnchorDate(toDate);
        return interpolator->getRate(yf);
    }

    vector<double> CalendarMonthAverageCurve::getRate(vector<QuantLib::Date> toDates) const
    {
        vector<double> rates = vector<double>(toDates.size());
        size_t counter = 1;
        Date refDate = toDates[0];
        double rate = getRate(refDate);
        rates[0] = rate;
        while (counter < toDates.size())
        {
            if (refDate.month() == toDates[counter].month() && refDate.year() == toDates[counter].year())
            {
                rates[counter] = rate;
            }
            else
            {
                refDate = toDates[counter];
                rate = getRate(refDate);
                rates[counter] = rate;
            }
            ++counter;
        }
        return rates;
    }

    vector<double> CalendarMonthAverageCurve::getObservationPoints() const
    {
        return observationPoints;
    }

    vector<Date> CalendarMonthAverageCurve::getObservationDates() const
    {
        return observationDates;
    }

    int CalendarMonthAverageCurve::getNumberOfNumberOfMonthsBetween(Date fromDate, Date toDate)
    {
        Date beginningOfFromMonth = Date(1, fromDate.month(), fromDate.year());
        Date beginningOfToMonth = Date(1, toDate.month(), toDate.year());
        std::shared_ptr<DayCounter> dayCounter = std::make_shared<Actual365Fixed>();
        double yf = dayCounter->yearFraction(beginningOfFromMonth, beginningOfToMonth);
        int monthsAsInt = boost::math::iround(yf * 12); // need to keep whole number of months ahead
        return monthsAsInt;
    }

    int CalendarMonthAverageCurve::getNumberOfMonthsAheadOfAnchorDate(Date date) const
    {
        Date beginningOfMonth = Date(1, date.month(), date.year());
        double yf = a365DayCounter->yearFraction(beginningOfAnchorDateMonth, beginningOfMonth);
        int monthsAsInt = boost::math::iround(yf * 12); // need to keep whole number of months ahead
        return monthsAsInt;
    }
}