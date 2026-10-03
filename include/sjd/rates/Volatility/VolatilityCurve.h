#ifndef SJD_VOLATILITYCURVE_INCLUDED
#define SJD_VOLATILITYCURVE_INCLUDED
#pragma once

#include <sjd/tools/DatedCurve.h>
#include <sjd/rates/Volatility/DeterministicVolatilityRateSource.h>

#include <ql/time/daycounters/simpledaycounter.hpp>
#include <ql/time/calendars/nullcalendar.hpp>

namespace sjd 
{

    /*======================================================================================
    VolatilityCurve

    Use this for European Volatilities observable on specific dates.
    NOTE 1) Interpolation happens in the Standard Deviation Space and not the Volatility 
            Space
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

        virtual void getVolAndSD(QuantLib::Date toDate, double &vol, double& sd) const;
        virtual void getVolAndSD(QuantLib::Date toDate, double strike, double forward, double &vol, double& sd) const;

        std::vector<QuantLib::Date> getObservationDates() const;
        ArrayInterpolatorType getType();

        virtual std::shared_ptr<VolatilityRateSource> parallelBump(double spread);
        virtual std::shared_ptr<VolatilityRateSource> rollForward(QuantLib::Date toDate);

    private:
        std::shared_ptr<DatedCurve> datedCurve;
        ArrayInterpolatorType type;
    };
}

#endif