#include <sjd/rates/Volatility/VolatilityCurve.h>

#include <algorithm>
#include <cmath>

#include <boost/lexical_cast.hpp> // convert numbers to string

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    /*======================================================================================
    VolatilityCurve
    =======================================================================================*/
    VolatilityCurve::VolatilityCurve(Date anchorDateInput,
                                     vector<Date> observationDatesInput,
                                     vector<double> volatilityInput,
                                     std::shared_ptr<DayCounter> dayCounterInput,
                                     ArrayInterpolatorType typeInput,
                                     bool allowExtrapolationInput)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("VolatilityCurve"));
        dayCounter = dayCounterInput;
        inputVolatilities = volatilityInput;
        anchorVolatility = volatilityInput.empty() ? 0.0 : volatilityInput.front();

        vector<double> standardDeviations(volatilityInput.size(), 0.0);
        bool anchorSupplied = false;
        for (size_t i = 0; i < volatilityInput.size() && i < observationDatesInput.size(); ++i)
        {
            if (observationDatesInput[i] == anchorDateInput)
            {
                anchorVolatility = volatilityInput[i];
                anchorSupplied = true;
                standardDeviations[i] = 0.0;
            }
            else
            {
                double yearFraction = dayCounter->yearFraction(anchorDateInput, observationDatesInput[i]);
                standardDeviations[i] = volatilityInput[i] * std::sqrt(std::max(yearFraction, 0.0));
            }
        }
        if (!anchorSupplied && !volatilityInput.empty())
        {
            anchorVolatility = volatilityInput.front();
        }

        datedCurve = std::shared_ptr<DatedCurve>(new DatedCurve(
            anchorDateInput, observationDatesInput, standardDeviations, typeInput, allowExtrapolationInput));

        anchorDate = datedCurve->getAnchorDate();
        allowExtrapolation = datedCurve->getAllowsExtrapolation();
        finalDate = datedCurve->getFinalDate();
        type = typeInput;
        string tmp;
        if (datedCurve->isOK(tmp) && (!datedCurve->containsRateOnAnchorDate()))
        {
            datedCurve->addPointOnAnchorDate(0.0);
        }
    }

    VolatilityCurve::~VolatilityCurve() {}

    bool VolatilityCurve::isOK()
    {
        DeterministicVolatilityRateSource::isOK();
        string errorMessage = "";
        if (!datedCurve->isOK(errorMessage))
        {
            const std::string code = errorMessage.find("same dimension") != std::string::npos
                                         ? "VOLATILITY_CURVE_DIMENSION_MISMATCH"
                                         : "VOLATILITY_CURVE_INVALID";
            recordIssue(code, errorMessage, {"inputs"});
            return false;
        }
        for (size_t i = 0; i < inputVolatilities.size(); ++i)
        {
            if (inputVolatilities[i] <= 0)
            {
                recordIssue("VOLATILITY_CURVE_NEGATIVE_INPUT", "has negative inputs", {"volatilities"});
            }
        }
        return !errorTracking->getHasErrors();
    }

    vector<Date> VolatilityCurve::getObservationDates()
    {
        return datedCurve->getObservationDates();
    }

    bool VolatilityCurve::isInRange(Date date) const
    {
        return RateSource::isInRange(date);
    }

    bool VolatilityCurve::isInRange(Date date, double strike, double forward) const
    {
        return RateSource::isInRange(date);
    }

    double VolatilityCurve::getVolatility(Date toDate) const
    {
        if (toDate == anchorDate)
        {
            return anchorVolatility;
        }
        double yearFraction = dayCounter->yearFraction(anchorDate, toDate);
        if (yearFraction <= 0.0)
        {
            return anchorVolatility;
        }
        return getStandardDeviation(toDate) / std::sqrt(yearFraction);
    }

    double VolatilityCurve::getVolatility(Date toDate, double strike, double forward) const
    {
        return getVolatility(toDate);
    }

    double VolatilityCurve::getStandardDeviation(Date toDate) const
    {
        return datedCurve->getRate(toDate);
    }

    double VolatilityCurve::getStandardDeviation(Date toDate, double strike, double forward) const
    {
        return getStandardDeviation(toDate);
    }

    void VolatilityCurve::getVolAndSD(Date toDate, double& vol, double& sd) const
    {
        vol = getVolatility(toDate);
        sd = getStandardDeviation(toDate);
        return;
    }

    void VolatilityCurve::getVolAndSD(
        Date toDate, double strike, double forward, double& vol, double& sd) const
    {
        return getVolAndSD(toDate, vol, sd);
    };

    vector<Date> VolatilityCurve::getObservationDates() const
    {
        return datedCurve->getObservationDates();
    }

    ArrayInterpolatorType VolatilityCurve::getType()
    {
        return type;
    }

    std::shared_ptr<VolatilityRateSource> VolatilityCurve::parallelBump(double spread)
    {
        vector<Date> rolledObservationDates;
        vector<double> rolledVols;
        for (size_t i = 0; i < getObservationDates().size(); ++i)
        {
            rolledObservationDates.push_back(getObservationDates()[i]);
            rolledVols.push_back(getVolatility(getObservationDates()[i]) + spread);
        }

        return std::shared_ptr<VolatilityCurve>(
            new VolatilityCurve(anchorDate, rolledObservationDates, rolledVols, dayCounter,
                                datedCurve->getType(), allowExtrapolation));
    }

    std::shared_ptr<VolatilityRateSource> VolatilityCurve::rollForward(Date toDate)
    {
        vector<Date> rolledObservationDates;
        vector<double> rolledVols;
        if (!isInRange(toDate))
        {
            rolledObservationDates.push_back(toDate);
            rolledVols.push_back(1.0);
        }
        else
        {
            for (size_t i = 0; i < getObservationDates().size(); ++i)
            {
                if (getObservationDates()[i] >= toDate)
                {
                    rolledObservationDates.push_back(getObservationDates()[i]);
                    rolledVols.push_back(getVolatility(getObservationDates()[i]));
                }
            }
        }
        return std::shared_ptr<VolatilityCurve>(
            new VolatilityCurve(toDate, rolledObservationDates, rolledVols, dayCounter, datedCurve->getType(),
                                allowExtrapolation));
    }
}