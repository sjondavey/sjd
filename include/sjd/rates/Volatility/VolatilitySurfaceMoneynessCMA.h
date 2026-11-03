#pragma once

#include "VolatilitySurfaces.h"

namespace sjd
{
    /*======================================================================================
    VolatilitySurfaceMoneynessCMA

    This class replicates how we are using the QuantLib::Calendar QuantLib::Month Average Volatility in
    Prototrader. There is no provision for extrapolation in either the time or moneyness
    dimensions.

    Note: rolling forward in time
    =======================================================================================*/
    class VolatilitySurfaceMoneynessCMA : public VolatiltiySurfaceRateSource
    {
    public:
        VolatilitySurfaceMoneynessCMA(QuantLib::Date anchorDate,
                                      std::vector<QuantLib::Date> observationDates,
                                      std::vector<double> moneyness,
                                      QuantLib::Matrix volatilityData,
                                      SurfaceInterpolatorType interpolatorType,
                                      bool extrapolate = true); // row = moneyness, column = time

        VolatilitySurfaceMoneynessCMA(QuantLib::Date anchorDate,
                                      std::vector<QuantLib::Date> observationDates,
                                      std::vector<double> moneyness,
                                      std::vector<std::vector<double>> volatilityData,
                                      SurfaceInterpolatorType interpolatorType,
                                      bool extrapolate = true); // row = moneyness, column = time

        virtual ~VolatilitySurfaceMoneynessCMA() {};

        virtual void setInputs(QuantLib::Date anchorDate,
                               std::vector<QuantLib::Date> observationDates,
                               std::vector<double> moneyness,
                               QuantLib::Matrix volatilityData,
                               SurfaceInterpolatorType interpolatorType,
                               bool extraploate);

        void runChecks() override;

        double getMonthsAheadOfAnchorDate(QuantLib::Date toDate) const;

        virtual bool isInRange(QuantLib::Date date) const;
        virtual bool isInRange(QuantLib::Date date, double strike, double forward) const;
        virtual bool extendsTo(QuantLib::Date date) const;

        virtual double getVolatility(QuantLib::Date toDate) const;
        virtual double getVolatility(QuantLib::Date toDate, double strike, double forward) const;
        // I need to over write these to deal with different day counters
        virtual double getStandardDeviation(QuantLib::Date toDate) const;
        virtual double getStandardDeviation(QuantLib::Date toDate, double strike, double forward) const;

        virtual void getVolAndSD(
            QuantLib::Date toDate, double strike, double forward, double& vol, double& sd) const;
        virtual void getVolAndSD(std::vector<QuantLib::Date> toDates,
                                 double strike,
                                 std::vector<double> forwards,
                                 std::vector<double>& vols,
                                 std::vector<double>& sds) const;

        virtual std::shared_ptr<VolatilityRateSource> rollForward(QuantLib::Date toDate);
        virtual std::shared_ptr<VolatilityRateSource> parallelBump(double spread);

        virtual void getRolledVariables(QuantLib::Date rolltoDate,
                                        std::vector<QuantLib::Date>& rolledObservationDates,
                                        QuantLib::Matrix& rolledVols);

    protected:
        QuantLib::Date beginningOfAnchorDateMonth;
        std::shared_ptr<QuantLib::DayCounter> simpleDayCounter;
    };
}
