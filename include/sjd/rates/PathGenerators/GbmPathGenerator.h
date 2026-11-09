#pragma once

#include <math.h>

#include <boost/math/distributions/normal.hpp>
#include <cstdint>
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
    // Box-Muller on std::mt19937. Two uniforms per normal, the sequence the golden paths use.
    class Mt19937Normals
    {
    public:
        explicit Mt19937Normals(long seed);
        double next();
        void discardNormals(std::uint64_t count);

    private:
        std::mt19937 rng;
    };

    // One path buffer. The curves it rolls are not shared with another workspace.
    struct PathWorkspace
    {
        std::shared_ptr<ForwardRateSourceStochastic> curve;
        std::shared_ptr<Path> path;
        std::vector<std::shared_ptr<ForwardRateSource>> forwards;
        std::vector<std::shared_ptr<RollingFixingRateSource>> fixings;
        std::vector<QuantLib::Date> fixingDates;
        std::vector<double> fixingRates;
        std::vector<double> shocks;
        bool usesInterestDifferential = false;
    };

    /*======================================================================================
    EconomicScenarioGenerator
    =======================================================================================*/
    class EconomicScenarioGenerator : public Validatable
    {
    public:
        void runChecks() override;
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
        size_t numDates = 0;
    };

    /*======================================================================================
    GbmPathGenerator

    One geometric Brownian factor. Discount and volatility roll deterministically.
    The forward rolls with the shocks passed into generatePath. factors() is 1.
    =======================================================================================*/
    class GbmPathGenerator : public EconomicScenarioGenerator
    {
    public:
        GbmPathGenerator(const std::shared_ptr<RatesEnvironment> re);

        virtual void setBaseRates(const std::shared_ptr<RatesEnvironment> re);
        void runChecks() override;

        // returns a false if there was any problem setting the dates and calculating the values used in the
        // simulation
        virtual bool setSimulationDates(std::set<QuantLib::Date> simulationDates);

        virtual void setRandomSeed(long seed);
        long getRandomSeed() const;
        size_t factors() const;

        // A path buffer whose rolled curves are not the template's.
        PathWorkspace workspace();

        // Draws the next block of normals and writes them into the workspace.
        std::shared_ptr<Path> generatePath(Mt19937Normals& rng, PathWorkspace& space);
        // Negates the shocks just written. Does not draw again.
        std::shared_ptr<Path> generateAntithetic(PathWorkspace& space);
        std::shared_ptr<Path> writeShocks(const std::vector<double>& shocks, PathWorkspace& space);
        std::shared_ptr<Path> generateSpot(const std::vector<double>& spot, PathWorkspace& space);

        // 0 < confidenceInterval < 1. Uses the template path, for prepare().
        virtual std::shared_ptr<Path> getConfidenceInterval(double confidenceInterval);
        virtual std::shared_ptr<Path> getRandomPath();
        virtual std::shared_ptr<Path> getAntitheticPath();
        virtual std::shared_ptr<Path> getPathForRandomNumber(std::vector<double> randomNumbers);
        virtual std::shared_ptr<Path> getPathForSpot(std::vector<double> spot);

    protected:
        void updateFXForwardWithRolledDiscountRateSources(size_t stepNumber);
        bool frsDependsOnInterestRateDifferential = false;

        std::shared_ptr<DeterministicDiscountRateSource> drs;
        std::shared_ptr<DeterministicDiscountRateSource> foreigndrs;
        std::shared_ptr<DeterministicVolatilityRateSource> vrs;
        std::shared_ptr<ForwardRateSourceStochastic> frs;
        std::shared_ptr<RollingFixingRateSource> fixingrs;
        std::vector<QuantLib::Date> allFixingDates;
        std::vector<double> allFixingRates;

        double drawStandardNormal();

        long seed = 0;
        Mt19937Normals normals{0};
        std::vector<double> randomNumbers;
    };

    using SingleFactorEconomicScenarioGenerator = GbmPathGenerator;

}
