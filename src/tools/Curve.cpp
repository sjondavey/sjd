#include <sjd/tools/Curve.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
   Curve
    =======================================================================================*/
    Curve::Curve()
    {
        errorTracking = std::make_shared<IssueTracker>("Curve");
    }

    Curve::Curve(Date anchorDateInput,
                 vector<double> observationPointsInput,
                 vector<double> observedRatesInput,
                 ArrayInterpolatorType typeInput,
                 bool allowExtrapolationInput)
    {
        errorTracking = std::make_shared<IssueTracker>("Curve");
        setParameters(anchorDateInput, observationPointsInput, observedRatesInput, typeInput,
                      allowExtrapolationInput);
    }

    void Curve::setParameters(Date anchorDateInput,
                              vector<double> observationPointsInput,
                              vector<double> observedRatesInput,
                              ArrayInterpolatorType typeInput,
                              bool allowExtrapolationInput)
    {
        type = typeInput;
        observationPoints = observationPointsInput;
        observedRates = observedRatesInput;
        anchorDate = anchorDateInput;
        allowExtrapolation = allowExtrapolationInput;

        // Other errors handled by ArrayInterpolator
        if (observationPoints.size() < 1 || observedRates[0] < 0)
        {
            interpolator = std::make_shared<LinearArrayInterpolator>(
                vector<double>(1, 0), vector<double>(1, 1), allowExtrapolation);
            return;
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

    void Curve::runChecks()
    {
        if (observationPoints.size() < 1)
        {
            recordIssue(issues::CURVE_INPUT_EMPTY, "Input time empty", {});
            return;
        }
        if (observationPoints[0] < 0)
        {
            recordIssue(issues::CURVE_OBSERVATION_BEFORE_ZERO, "First ovservation time before zero", {});
            return;
        }
        if (interpolator && !interpolator->isOk())
            absorbChildReport(interpolator->validationReport(), {});
    }

    Date Curve::getAnchorDate() const
    {
        return anchorDate;
    }

    ArrayInterpolatorType Curve::getType()
    {
        return type;
    }

    bool Curve::getAllowsExtrapolation()
    {
        return allowExtrapolation;
    }

    vector<double> Curve::getRates()
    {
        return observedRates;
    }

    void Curve::setRates(vector<double> updatedRates)
    {
        observedRates = updatedRates;
        interpolator->setYVector(updatedRates);
    }

    bool Curve::containsRateOnAnchorDate()
    {
        return (abs(observationPoints[0]) < 1e-14);
    }

    void Curve::addPointOnAnchorDate(double valueToAdd)
    {
        vector<double> observationPointsInput = observationPoints;
        vector<double> observedRatesInput = observedRates;
        observationPointsInput.insert(observationPointsInput.begin(), 0);
        observedRatesInput.insert(observedRatesInput.begin(), valueToAdd);

        setParameters(anchorDate, observationPointsInput, observedRatesInput, type, allowExtrapolation);
    }

    double Curve::getRate(double toTime) const
    {
        return interpolator->getRate(toTime);
    }

    vector<double> Curve::getObservationPoints() const
    {
        return observationPoints;
    }

}