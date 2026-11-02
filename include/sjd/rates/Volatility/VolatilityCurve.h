#pragma once

#include <sjd/rates/Volatility/DeterministicVolatilityRateSource.h>
#include <sjd/tools/DatedCurve.h>

#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/daycounters/simpledaycounter.hpp>

namespace sjd
{

    /*======================================================================================
    VolatilityCurve

    Use this for European Volatilities observable on specific dates.
    NOTE 1) The curve stores total standard deviation, volatility times the square
            root of the year fraction from the anchor, and interpolates that.
            getVolatility divides the interpolated standard deviation by the
            square root of the year fraction. On the anchor date the volatility
            is the volatility supplied for that date, or the first input
            volatility when the anchor is not itself an observation date.
         2) There are no calendar objects here, all dates are assumed to be good dates
            by the time they hit this object
    =======================================================================================*/
    class VolatilityCurve : public DeterministicVolatilityRateSource
    {
    public:
        VolatilityCurve(QuantLib::Date anchorDate,
                        std::vector<QuantLib::Date> observationDates,
                        std::vector<double> volatility,
                        std::shared_ptr<QuantLib::DayCounter> dayCounter,
                        ArrayInterpolatorType type = LINEAR,
                        bool allowExtrapolation = false);

        virtual ~VolatilityCurve();

        virtual bool isOK();
        virtual std::string validationRoot() const { return "volatility_curve"; }
        virtual std::vector<QuantLib::Date> getObservationDates();

        virtual bool isInRange(QuantLib::Date date) const;
        virtual bool isInRange(QuantLib::Date date, double strike, double forward) const;

        virtual double getVolatility(QuantLib::Date toDate) const;
        virtual double getVolatility(QuantLib::Date toDate, double strike, double forward) const;

        virtual double getStandardDeviation(QuantLib::Date toDate) const;
        virtual double getStandardDeviation(QuantLib::Date toDate, double strike, double forward) const;

        virtual void getVolAndSD(QuantLib::Date toDate, double& vol, double& sd) const;
        virtual void getVolAndSD(
            QuantLib::Date toDate, double strike, double forward, double& vol, double& sd) const;

        std::vector<QuantLib::Date> getObservationDates() const;
        ArrayInterpolatorType getType();

        virtual std::shared_ptr<VolatilityRateSource> parallelBump(double spread);
        virtual std::shared_ptr<VolatilityRateSource> rollForward(QuantLib::Date toDate);

    private:
        std::shared_ptr<DatedCurve> datedCurve;
        ArrayInterpolatorType type;
        double anchorVolatility;
        std::vector<double> inputVolatilities;
    };
}
