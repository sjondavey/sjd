#include <sjd/tools/DatedCurve.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    DatedCurve
    =======================================================================================*/
    DatedCurve::DatedCurve()
    {
        errorTracking = std::make_shared<IssueTracker>("DatedCurve");
    }

    DatedCurve::DatedCurve(Date anchorDateInput,
                           vector<Date> observationDatesInput,
                           vector<double> observedRatesInput,
                           ArrayInterpolatorType typeInput,
                           bool allowExtrapolationInput)
    {
        errorTracking = std::make_shared<IssueTracker>("DatedCurve");
        setParameters(anchorDateInput, observationDatesInput, observedRatesInput, typeInput,
                      allowExtrapolationInput);
    }

    void DatedCurve::setParameters(Date anchorDateInput,
                                   vector<Date> observationPointsInput,
                                   vector<double> observedRatesInput,
                                   ArrayInterpolatorType typeInput,
                                   bool allowExtrapolationInput)
    {
        type = typeInput;
        observationDates = observationPointsInput;
        observedRates = observedRatesInput;
        anchorDate = anchorDateInput;
        allowExtrapolation = allowExtrapolationInput;

        // Other errors handled by ArrayInterpolator
        if (observationDates.size() < 1)
        {
            finalDate = anchorDate;
            interpolator = std::make_shared<LinearArrayInterpolator>(
                vector<double>(1, 0), vector<double>(1, 1), allowExtrapolation);
            return;
        }
        finalDate = observationPointsInput.back();
        observationPoints.clear();
        for (size_t i = 0; i < observationPointsInput.size(); ++i)
        {
            observationPoints.push_back(observationPointsInput[i].serialNumber());
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

    void DatedCurve::runChecks()
    {
        if (observationPoints.size() < 1)
        {
            recordIssue(issues::CURVE_INPUT_EMPTY, "Input time empty", {});
            return;
        }
        if (observationDates[0] < anchorDate)
        {
            recordIssue(issues::CURVE_OBSERVATION_BEFORE_ANCHOR, "First ovservation time before Anchor Date",
                        {});
            return;
        }
        if (interpolator && !interpolator->isOk())
            absorbChildReport(interpolator->validationReport(), {});
    }

    void DatedCurve::setRates(vector<double> updatedRates)
    {
        observedRates = updatedRates;
        interpolator->setYVector(updatedRates);
    }

    bool DatedCurve::containsRateOnAnchorDate()
    {
        return (observationDates[0] == anchorDate);
    }

    void DatedCurve::addPointOnAnchorDate(double valueToAdd)
    {
        vector<Date> observationPointsInput = observationDates;
        vector<double> observedRatesInput = observedRates;
        observationPointsInput.insert(observationPointsInput.begin(), anchorDate);
        observedRatesInput.insert(observedRatesInput.begin(), valueToAdd);

        setParameters(anchorDate, observationPointsInput, observedRatesInput, type, allowExtrapolation);
    }

    double DatedCurve::getRate(Date toDate) const
    {
        return interpolator->getRate(toDate.serialNumber());
    }

    Date DatedCurve::getAnchorDate() const
    {
        return anchorDate;
    }

    Date DatedCurve::getFinalDate()
    {
        return finalDate;
    }

    ArrayInterpolatorType DatedCurve::getType()
    {
        return type;
    }

    vector<Date> DatedCurve::getObservationDates() const
    {
        return observationDates;
    }

    vector<double> DatedCurve::getRates()
    {
        return observedRates;
    }

    bool DatedCurve::getAllowsExtrapolation()
    {
        return allowExtrapolation;
    }
}