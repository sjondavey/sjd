#ifndef SJD_DISCOUNTINGRATESOURCE_INCLUDED
#define SJD_DISCOUNTINGRATESOURCE_INCLUDED
#pragma once

#include <sjd/rates/RateSource.h>

#include <ql/compounding.hpp>
#include <ql/time/frequency.hpp>

namespace sjd
{
    /**
     * Moves cashflows in time. The discount factor on the anchor date is 1,
     * including when the input curve has no point on that date.
     *
     * getDiscountFactor assumes the date was checked with isInRange. It does
     * not check again.
     */
    class DiscountRateSource : public RateSource
    {

    public:
        virtual ~DiscountRateSource() {};

        // NB Dates assumed to be good. Call isInRange(...) before calling this
        virtual double getDiscountFactor(QuantLib::Date toDate) const = 0;
        virtual double getDiscountFactor(QuantLib::Date fromDate, QuantLib::Date toDate) const;
        virtual std::vector<double> getDiscountFactor(std::vector<QuantLib::Date> times) const;
        virtual std::vector<double> getDiscountFactor(QuantLib::Date fromTime,
                                                      std::vector<QuantLib::Date> times) const;
    };
}

#endif