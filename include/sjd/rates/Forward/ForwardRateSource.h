#ifndef SJD_FORWARDRATESOURCE_INCLUDED
#define SJD_FORWARDRATESOURCE_INCLUDED
#pragma once

#include <sjd/rates/RateSource.h>
#include <sjd/rates/Volatility/VolatilityRateSource.h>
#include <vector>

namespace sjd
{
    /**
     * A forward or futures curve: an interpolated curve, a calendar-month average,
     * or a spot plus two discount curves (FX).
     *
     * From the anchor date through the first observation, the forward is flat at
     * the first observation. Do not extrapolate backwards.
     *
     * getForward assumes the date was checked with isInRange.
     */
    class ForwardRateSource : public RateSource
    {
    public:
        virtual ~ForwardRateSource() {};

        virtual double getForward(QuantLib::Date toDate) const = 0;
        virtual std::vector<double> getForward(std::vector<QuantLib::Date> toDate) const;

        // A forward forward is the factor that is applied to get the forward to T2 given
        // the forward to T1 where T0 <= T1 <= T2
        virtual double getForwardFworwardAdditive(QuantLib::Date fromDate, QuantLib::Date toDate) const;
        virtual double getForwardFworwardMultiplicative(QuantLib::Date fromDate, QuantLib::Date toDate) const;
        virtual std::vector<double> getForwardFworwardAdditive(QuantLib::Date fromDate,
                                                               std::vector<QuantLib::Date> toDates) const;
        virtual std::vector<double> getForwardFworwardMultiplicative(
            QuantLib::Date fromDate, std::vector<QuantLib::Date> toDates) const;
    };

}

#endif