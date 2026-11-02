#include <sjd/tools/Curve.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    /*======================================================================================
   Curve
    =======================================================================================*/
    Curve::Curve() {}

    Curve::Curve(Date anchorDateInput,
                 vector<double> observationPointsInput,
                 vector<double> observedRatesInput,
                 ArrayInterpolatorType typeInput,
                 bool allowExtrapolationInput)
    {
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
            interpolator = std::shared_ptr<ArrayInterpolator>(
                new LinearArrayInterpolator(vector<double>(1, 0), vector<double>(1, 1), allowExtrapolation));
            return;
        }

        switch (type)
        {
        case CUBIC:
            interpolator = std::shared_ptr<ArrayInterpolator>(
                new CubicSplineInterpolator(observationPoints, observedRates, allowExtrapolation));
            break;
        default:
            interpolator = std::shared_ptr<ArrayInterpolator>(
                new LinearArrayInterpolator(observationPoints, observedRates, allowExtrapolation));
        }
    }

    bool Curve::isOK(string& errorMessage)
    {
        errorMessage = "";
        if (observationPoints.size() < 1)
        {
            errorMessage = "Input time empty";
            return false;
        }
        if (observationPoints[0] < 0)
        {
            errorMessage = "First ovservation time before zero";
            return false;
        }
        if (!interpolator->isOk())
        {
            errorMessage = interpolator->getErrorMessage();
            return false;
        }
        return true;
    }

    Date Curve::getAnchorDate()
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