#ifndef SJD_VOLATILITYCURVECMA_INCLUDED
#define SJD_VOLATILITYCURVECMA_INCLUDED
#pragma once

#include <sjd/tools/CalendarMonthAverageCurve.h>
#include <sjd/rates/Volatility/DeterministicVolatilityRateSource.h>

#include <ql/time/daycounters/simpledaycounter.hpp>
#include <ql/time/calendars/nullcalendar.hpp>

namespace sjd 
{
    /*======================================================================================
    VolatilityCurveCMA

    Use this for volatility curves where every date in a single calendar month has the same 
    volatility
    =======================================================================================*/
    class VolatilityCurveCMA : public DeterministicVolatilityRateSource
    {
    public:
        
        VolatilityCurveCMA(QuantLib::Date anchorDate, 
                        std::vector<QuantLib::Date> observationDates, 
                        std::vector<double> volatility, 
                        std::shared_ptr<QuantLib::DayCounter> dayCounter,
                        ArrayInterpolatorType type = LINEAR,
                        bool allowExtrapolation = false);    
        
        virtual ~VolatilityCurveCMA();

        virtual bool isOK();
        virtual std::string validationRoot() const { return "volatility_curve_cma"; }
        virtual std::vector<QuantLib::Date> getObservationDates();

        virtual bool isInRange(QuantLib::Date date) const;
        virtual bool isInRange(QuantLib::Date date, double strike, double forward) const;
        virtual bool extendsTo(QuantLib::Date date) const;        

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
        std::shared_ptr<CalendarMonthAverageCurve> curve;
        ArrayInterpolatorType type;
    };
}

#endif