#include <sjd/rates/Volatility/VolatilityCurveCMA.h>

#include <boost/lexical_cast.hpp> // convert numbers to string

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    VolatilityCurve
    =======================================================================================*/
    VolatilityCurveCMA::VolatilityCurveCMA(Date anchorDateInput,
                                           vector<Date> observationDatesInput,
                                           vector<double> volatilityInput,
                                           std::shared_ptr<DayCounter> dayCounterInput,
                                           ArrayInterpolatorType typeInput,
                                           bool allowExtrapolationInput)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("VolatilityCurveCMA"));

        curve = std::shared_ptr<CalendarMonthAverageCurve>(new CalendarMonthAverageCurve(
            anchorDateInput, observationDatesInput, volatilityInput, typeInput, allowExtrapolationInput));

        anchorDate = curve->getAnchorDate();
        allowExtrapolation = curve->getAllowsExtrapolation();
        finalDate = curve->getFinalDate();
        type = typeInput;
        string tmp;
        if (curve->isOK(tmp) && (!curve->containsRateOnAnchorDate()))
        {
            curve->addPointOnAnchorDate(volatilityInput[0]);
        }
        dayCounter = dayCounterInput;
    }

    VolatilityCurveCMA::~VolatilityCurveCMA() {}

    bool VolatilityCurveCMA::isOK()
    {
        DeterministicVolatilityRateSource::isOK();
        string errorMessage = "";
        if (!curve->isOK(errorMessage))
        {
            const std::string code = errorMessage.find("same dimension") != std::string::npos
                                         ? "VOLATILITY_CURVE_CMA_DIMENSION_MISMATCH"
                                         : "VOLATILITY_CURVE_CMA_INVALID";
            recordIssue(code, errorMessage, {"inputs"});
            return false;
        }
        for (size_t i = 0; i < curve->getObservationDates().size(); ++i)
        {
            bool hasNegativeInputs = false;
            if ((!hasNegativeInputs) && (curve->getRates()[i] <= 0))
            {
                recordIssue("VOLATILITY_CURVE_CMA_NEGATIVE_INPUT", "has negative inputs", {"volatilities"});
                hasNegativeInputs = true;
            }
        }
        return !errorTracking->getHasErrors();
    }

    vector<Date> VolatilityCurveCMA::getObservationDates()
    {
        return curve->getObservationDates();
    }

    bool VolatilityCurveCMA::isInRange(Date d) const
    {
        Date beginningOfMonth = Date(1, d.month(), d.year());
        Date beginningOfAnchorDate = Date(1, anchorDate.month(), anchorDate.year());
        if (allowExtrapolation)
        {
            return (beginningOfMonth >= beginningOfAnchorDate);
        }
        else
        {
            return ((beginningOfMonth >= beginningOfAnchorDate) && (beginningOfMonth <= finalDate));
        }
    }

    bool VolatilityCurveCMA::extendsTo(Date d) const
    {
        Date beginningOfMonth = Date(1, d.month(), d.year());
        return RateSource::extendsTo(beginningOfMonth);
    }

    bool VolatilityCurveCMA::isInRange(Date date, double strike, double forward) const
    {
        return isInRange(date);
    }

    double VolatilityCurveCMA::getVolatility(Date toDate) const
    {
        return curve->getRate(toDate);
    }

    double VolatilityCurveCMA::getVolatility(Date toDate, double strike, double forward) const
    {
        return getVolatility(toDate);
    }

    double VolatilityCurveCMA::getStandardDeviation(Date toDate) const
    {
        double yearFraction = dayCounter->yearFraction(anchorDate, toDate);
        return curve->getRate(toDate) * sqrt(yearFraction);
    }

    double VolatilityCurveCMA::getStandardDeviation(Date toDate, double strike, double forward) const
    {
        return getStandardDeviation(toDate);
    }

    void VolatilityCurveCMA::getVolAndSD(Date toDate, double& vol, double& sd) const
    {
        vol = curve->getRate(toDate);
        double yearFraction = dayCounter->yearFraction(anchorDate, toDate);
        sd = vol * sqrt(yearFraction);
        return;
    }

    void VolatilityCurveCMA::getVolAndSD(
        Date toDate, double strike, double forward, double& vol, double& sd) const
    {
        return getVolAndSD(toDate, vol, sd);
    };

    vector<Date> VolatilityCurveCMA::getObservationDates() const
    {
        return curve->getObservationDates();
    }

    ArrayInterpolatorType VolatilityCurveCMA::getType()
    {
        return type;
    }

    std::shared_ptr<VolatilityRateSource> VolatilityCurveCMA::parallelBump(double spread)
    {
        vector<Date> rolledObservationDates;
        vector<double> rolledVols;
        for (size_t i = 0; i < getObservationDates().size(); ++i)
        {
            rolledObservationDates.push_back(getObservationDates()[i]);
            rolledVols.push_back(getVolatility(getObservationDates()[i]) + spread);
        }

        return std::shared_ptr<VolatilityCurveCMA>(
            new VolatilityCurveCMA(anchorDate, rolledObservationDates, rolledVols, dayCounter,
                                   curve->getType(), allowExtrapolation));
    }

    std::shared_ptr<VolatilityRateSource> VolatilityCurveCMA::rollForward(Date toDate)
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
        return std::shared_ptr<VolatilityCurveCMA>(new VolatilityCurveCMA(
            toDate, rolledObservationDates, rolledVols, dayCounter, curve->getType(), allowExtrapolation));
    }
}