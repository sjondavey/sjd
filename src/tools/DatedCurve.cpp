#include <sjd/tools/DatedCurve.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    /*======================================================================================
    DatedCurve
    =======================================================================================*/
    DatedCurve::DatedCurve() {}

    DatedCurve::DatedCurve(Date anchorDateInput,
                           vector<Date> observationDatesInput,
                           vector<double> observedRatesInput,
                           ArrayInterpolatorType typeInput,
                           bool allowExtrapolationInput)
    {
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
            interpolator = std::shared_ptr<ArrayInterpolator>(
                new LinearArrayInterpolator(vector<double>(1, 0), vector<double>(1, 1), allowExtrapolation));
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
        case CUBIC:
            interpolator = std::shared_ptr<ArrayInterpolator>(
                new CubicSplineInterpolator(observationPoints, observedRates, allowExtrapolation));
            break;
        default:
            interpolator = std::shared_ptr<ArrayInterpolator>(
                new LinearArrayInterpolator(observationPoints, observedRates, allowExtrapolation));
        }
    }

    bool DatedCurve::isOK(string& errorMessage)
    {
        errorMessage = "";
        if (observationPoints.size() < 1)
        {
            errorMessage = "Input time empty";
            return false;
        }
        if (observationDates[0] < anchorDate)
        {
            errorMessage = "First ovservation time before Anchor Date";
            return false;
        }
        if (!interpolator->isOk())
        {
            errorMessage = interpolator->getErrorMessage();
            return false;
        }
        return true;
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

    Date DatedCurve::getAnchorDate()
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