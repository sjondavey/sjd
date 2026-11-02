#pragma once

#include <math.h>

#include <boost/math/distributions/normal.hpp>
#include <random>
#include <set>
#include <sjd/rates/RatesEnvironment.h>

#include <sjd/rates/Discount/DeterministicDiscountRateSource.h>
#include <sjd/rates/Fixing/RollingFixingRateSource.h>
#include <sjd/rates/Forward/ForwardRateSourceStochastic.h>
#include <sjd/rates/Forward/UsesInterestRateDifferential.h>
#include <sjd/rates/Volatility/DeterministicVolatilityRateSource.h>

#include "Path.h"

namespace sjd
{
    /*======================================================================================
    EconomicScenarioGenerator
    =======================================================================================*/
    class EconomicScenarioGenerator : public Validatable
    {
    public:
        bool isOK();
        QuantLib::Date getAnchorDate() const;

        virtual void setBaseRates(const std::shared_ptr<RatesEnvironment> re);
        std::shared_ptr<RatesEnvironment> getBaseRates();

        // returns a false if there was any problem setting the dates and calculating the values used in the
        // simulation
        virtual bool setSimulationDates(std::set<QuantLib::Date> simulationDates);

        virtual std::shared_ptr<Path> getRandomPath() = 0;
        virtual std::shared_ptr<Path> getAntitheticPath() = 0;

    protected:
        std::string validationRoot() const override;

        std::shared_ptr<RatesEnvironment> re;

        std::shared_ptr<Path> path;
        std::vector<std::shared_ptr<DiscountRateSource>> drss;
        std::vector<std::shared_ptr<DiscountRateSource>> foreigndrss;
        std::vector<std::shared_ptr<VolatilityRateSource>> vrss;
        std::vector<std::shared_ptr<ForwardRateSource>> frss;
        std::vector<std::shared_ptr<RollingFixingRateSource>> fixingrss;

        QuantLib::Date anchorDate;

        std::vector<QuantLib::Date> simulationDates;
        size_t numDates;
    };

    /*======================================================================================
    SingleFactorEconomicScenarioGenerator

    Has a deterministic discount and volatility rate source, a stochastic forward rate source
    and an optional fixing rate source
    =======================================================================================*/
    class SingleFactorEconomicScenarioGenerator : public EconomicScenarioGenerator
    {
    public:
        SingleFactorEconomicScenarioGenerator(const std::shared_ptr<RatesEnvironment> re);

        virtual void setBaseRates(const std::shared_ptr<RatesEnvironment> re);
        bool isOK();

        // returns a false if there was any problem setting the dates and calculating the values used in the
        // simulation
        virtual bool setSimulationDates(std::set<QuantLib::Date> simulationDates);

        virtual void setRandomSeed(long seed);
        long getRandomSeed() const;

        // 0 < confidenceInterval < 1
        virtual std::shared_ptr<Path> getConfidenceInterval(double confidenceInterval);
        virtual std::shared_ptr<Path> getRandomPath();
        virtual std::shared_ptr<Path> getAntitheticPath();
        virtual std::shared_ptr<Path> getPathForRandomNumber(std::vector<double> randomNumbers);
        virtual std::shared_ptr<Path> getPathForSpot(std::vector<double> spot);

    protected:
        void updateFXForwardWithRolledDiscountRateSources(size_t stepNumber);
        bool frsDependsOnInterestRateDifferential;

        std::shared_ptr<DeterministicDiscountRateSource> drs;
        std::shared_ptr<DeterministicDiscountRateSource> foreigndrs;
        std::shared_ptr<DeterministicVolatilityRateSource> vrs;
        std::shared_ptr<ForwardRateSourceStochastic> frs;
        std::shared_ptr<RollingFixingRateSource> fixingrs;
        std::vector<QuantLib::Date> allFixingDates;
        std::vector<double> allFixingRates;

        long seed = 0;
        std::mt19937 rng;
        std::normal_distribution<double> nd{0.0, 1.0};
        std::vector<double> randomNumbers;
    };

}
