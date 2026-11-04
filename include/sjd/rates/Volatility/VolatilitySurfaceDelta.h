#pragma once

#include "VolatilitySurfaces.h"

namespace sjd
{

    /*======================================================================================
    BlackPutImpliedStrike

    A classes built to utilise the QuantLib "solver framework" to imply the standard
    deviation an option given all the other inputs. These all now solve for Standard
    Deviation rather than Volatility. It is up to the user to do the simple conversion
    =======================================================================================*/
    class BlackPutImpliedStrike
    {
    public:
        BlackPutImpliedStrike(double delta, double forward, double standardDeviation)
            : delta(delta / 100), forward(forward), standardDeviation(standardDeviation)
        {
            put = std::make_shared<Black76Put>(forward, forward, standardDeviation, 1.0);
        };

        double operator()(double strikeEstimate) const
        {
            put->setStrike(strikeEstimate);
            [[maybe_unused]] double d = put->getDelta();
            return delta + put->getDelta(); // put delta is negative
        }

    private:
        double delta, forward, standardDeviation;
        std::shared_ptr<Black76Option> put;
    };

    /*======================================================================================
    VolatilitySurfaceDelta

    =======================================================================================*/
    class VolatilitySurfaceDelta : public VolatiltiySurfaceRateSource
    {
    public:
        // Note that delta should be for a put and therefore must be a strictly increasing std::vector.
        // Also use 50 NOT 0.5 for 50 delta options
        VolatilitySurfaceDelta(QuantLib::Date anchorDate,
                               std::vector<QuantLib::Date> observationDates,
                               std::vector<double> deltas,
                               std::vector<std::vector<double>> volatilityData,
                               SurfaceInterpolatorType interpolatorType,
                               bool extrapolate = false); // row = delta, column = time

        VolatilitySurfaceDelta(QuantLib::Date anchorDate,
                               std::vector<QuantLib::Date> observationDates,
                               std::vector<double> deltas,
                               QuantLib::Matrix volatilityData,
                               SurfaceInterpolatorType interpolatorType,
                               bool extrapolate = false); // row = delta, column = time

        virtual ~VolatilitySurfaceDelta() {};

        void runChecks() override;

        virtual bool isInRange(QuantLib::Date date) const;
        virtual bool isInRange(QuantLib::Date date, double strike, double forward) const;

        virtual double getVolatility(QuantLib::Date toDate) const;
        virtual double getVolatility(QuantLib::Date toDate, double strike, double forward) const;

        virtual double getStandardDeviation(QuantLib::Date toDate) const;
        virtual double getStandardDeviation(QuantLib::Date toDate, double strike, double forward) const;

        double calculateDeltaFromStrike(QuantLib::Date date, double strike, double forward) const;
        double calculateStrikeFromDelta(QuantLib::Date date, double delta, double forward) const;

        virtual std::shared_ptr<VolatilityRateSource> parallelBump(double spread);
        virtual std::shared_ptr<VolatilityRateSource> rollForward(QuantLib::Date toDate);
    };

    /*======================================================================================
    SimpleDeltaSurface

    Used for excel *formula* rather than excel *objects*
    =======================================================================================*/
    class SimpleDeltaSurface : public Validatable
    {
    public:
        SimpleDeltaSurface(std::vector<double> times,
                           std::vector<double> delta,
                           std::vector<std::vector<double>> volatility,
                           bool extrapolate,
                           std::string interpolationType);
        std::string validationRoot() const override { return "delta_surface"; }

        bool isInDeltaRange(double time, double delta);
        bool isInMoneynessRange(double time, double moneyness);

        double getVolatility(double time) const;
        double getVolatilityForDelta(double time, double delta) const;
        double getVolatilityForMoneyness(double time, double moneyness) const;

    protected:
        void runChecks() override;

    private:
        double calculateDeltaFromStrike(double forward, double strike, double time) const;

        bool extrapolate = false;
        std::vector<double> times, delta;
        std::vector<std::vector<double>> rows;
        QuantLib::Matrix volatilityMatrix;
        std::shared_ptr<QuantLib::Interpolation2D> interpolator;
        std::string interpolationType;
    };
}
