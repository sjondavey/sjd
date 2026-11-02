#pragma once

#include <sjd/rates/RateSource.h>
#include <vector>

namespace sjd
{
    /**
     * A volatility curve or surface. Pricing calls getStandardDeviation: that
     * number is unitless, so the formula does not need a day count.
     *
     * getVolatility and getStandardDeviation assume the date, and for a surface
     * the strike and forward, were checked with isInRange.
     */
    class VolatilityRateSource : public RateSource
    {
    public:
        virtual ~VolatilityRateSource() {};

        virtual bool isOK();

        std::shared_ptr<QuantLib::DayCounter> getDayCounter() const;

        using RateSource::isInRange;
        virtual bool isInRange(QuantLib::Date date, double strike, double forward) const = 0;

        // check to see if we can roll along the curve using "spot vol"
        bool hasNegativeATMFwdFwdVariances(QuantLib::Date& startOfNegVar, QuantLib::Date& endOfNegVar);
        virtual std::vector<QuantLib::Date> getObservationDates() = 0;
        bool hasNegativeATMFwdFwdVariances(std::vector<QuantLib::Date> checkDates,
                                           QuantLib::Date& startOfNegVar,
                                           QuantLib::Date& endOfNegVar);

        // NB Dates assumed to be good. Call isInRange(...) before calling this
        virtual double getVolatility(QuantLib::Date toDate) const = 0;
        virtual double getVolatility(QuantLib::Date toDate, double strike, double forward) const = 0;
        virtual double getStandardDeviation(QuantLib::Date toDate) const;
        virtual double getStandardDeviation(QuantLib::Date toDate, double strike, double forward) const;
        virtual void getVolAndSD(QuantLib::Date toDate, double& vol, double& sd) const;
        virtual void getVolAndSD(
            QuantLib::Date toDate, double strike, double forward, double& vol, double& sd) const;

        // Ensure the input vols and sds vectors are of the correct size before using this method
        virtual void getVolAndSD(std::vector<QuantLib::Date> toDates,
                                 double strike,
                                 std::vector<double> forwards,
                                 std::vector<double>& vols,
                                 std::vector<double>& sds) const;

    protected:
        std::shared_ptr<QuantLib::DayCounter> dayCounter;
    };

}
