#pragma once

#include <algorithm>
#include <set>

#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/rates/PathGenerators/GbmPathGenerator.h>

namespace sjd
{
    class CemNettingSet;

    enum class PathSampling
    {
        Pseudo,
        Sobol
    };

    // threads == 0 uses the hardware count. Aggregates keep per-step profiles and
    // do not store the per-trade or per-netting-set cubes. capital is the mean CEM
    // profile, scaled by capitalScale, and only when the run has netting sets.
    struct SimulationOptions
    {
        bool antithetic = false;
        bool byTrade = false;
        bool byNettingSet = false;
        size_t threads = 1;
        PathSampling sampling = PathSampling::Pseudo;
        bool aggregates = false;
        bool capital = false;
        double capitalScale = 0.08;
    };

    // Every simulated value. Statistics are computed outside the evolver.
    // The arrays are flat and row-major: spot, mtm and margin are [path][step],
    // byTrade is [path][step][trade] and byStructure is [path][step][netting set].
    // margin is empty unless the portfolio is margined. byTrade and
    // byStructure are empty unless they were requested. A failed
    // simulation has paths == 0. With a holding period, dates also hold the
    // margin reset dates. Exposure date k is step exposureSteps[k], and its
    // margin is read at step marginResetSteps[k].
    // When aggregated is set, the path cubes are empty and the profiles are filled.
    struct SimulatedPaths
    {
        std::vector<QuantLib::Date> dates;
        std::vector<size_t> exposureSteps;
        std::vector<size_t> marginResetSteps;
        size_t paths = 0;
        size_t steps = 0;
        size_t trades = 0;
        size_t structures = 0;
        bool antithetic = false;
        bool aggregated = false;
        std::vector<double> spot;
        std::vector<double> mtm;
        std::vector<double> margin;
        std::vector<double> byTrade;
        std::vector<double> byStructure;
        std::vector<double> meanSpot;
        std::vector<double> meanMtm;
        std::vector<double> expectedExposure;
        std::vector<double> potentialFutureExposure;
        std::vector<double> meanMargin;
        std::vector<double> meanCapital;
    };

    class PortfolioEvolver : public Validatable
    {
    public:
        // Use this for unmargined portfolios or portfolios that do not require holding-period calculations
        PortfolioEvolver(std::shared_ptr<Portfolio> portfolio,
                         std::shared_ptr<GbmPathGenerator> pathGenerator,
                         std::set<QuantLib::Date> forwardDates);

        // Use this for portfolios that require holding-period calculations
        PortfolioEvolver(std::shared_ptr<Portfolio> portfolio,
                         std::shared_ptr<GbmPathGenerator> pathGenerator,
                         std::set<QuantLib::Date> forwardDates,
                         std::shared_ptr<QuantLib::Calendar> calendarForHoldingPeriodCalcuation,
                         size_t holdingPeriodInBusinessDays);

        void runChecks() override;
        // Sets the simulation dates on the path generator and checks that the portfolio
        // can be valued on those dates. validate() does not do this.
        bool prepare();

        std::vector<QuantLib::Date> getSimulationDates();
        std::vector<QuantLib::Date> getExposureDates();

        // Priced when a path is requested by netting set. The portfolio does not hold them.
        void setNettingSets(std::vector<std::shared_ptr<Contract>> sets);

        // With antithetic paths, path i and path i + paths/2 use opposite normals, and an
        // odd request is rounded up. This overload is one thread and the default sequence.
        SimulatedPaths simulatePaths(unsigned long numberOfPaths,
                                     bool antithetic,
                                     bool byTrade,
                                     bool byStructure = false);

        SimulatedPaths simulatePaths(unsigned long numberOfPaths, const SimulationOptions& options);

        // One path marked to these forwards. spots has one value per simulation date.
        SimulatedPaths valueSpotPath(const std::vector<double>& spots,
                                     bool byTrade,
                                     bool byStructure = false);

        // Mark the book to many forward paths. spots is row-major [path][step],
        // length numberOfPaths * simulationSteps. Use the same spots across
        // candidate portfolios so common random numbers stay fixed.
        // threads == 0 uses the hardware count.
        SimulatedPaths valueSpotPaths(const std::vector<double>& spots,
                                      size_t numberOfPaths,
                                      bool byTrade,
                                      bool byStructure = false,
                                      size_t threads = 1);

    protected:
        std::vector<double> getSpotAlongAPath(std::shared_ptr<Path> path);
        std::vector<double> getMtMAlongAPath(std::shared_ptr<Path> path);
        virtual void evolvePortfolioAlongPath(std::shared_ptr<Path> path,
                                              std::vector<double>& spot,
                                              std::vector<double>& mtm,
                                              std::vector<double>& margin);
        void recordPath(SimulatedPaths& result,
                        size_t row,
                        std::shared_ptr<Path> path,
                        const std::vector<std::shared_ptr<SingleInstrumentContract>>& trades,
                        const std::vector<std::shared_ptr<Contract>>& structures);
        void addPathCapital(std::shared_ptr<Path> path,
                            size_t row,
                            const SimulatedPaths& result,
                            const std::vector<std::shared_ptr<CemNettingSet>>& sets,
                            double scale,
                            std::vector<double>& capitalSum) const;

        void setSimulationDates(std::set<QuantLib::Date> dates);
        SimulatedPaths prepareResult(size_t paths,
                                     bool antithetic,
                                     bool byTrade,
                                     bool byStructure,
                                     std::vector<std::shared_ptr<SingleInstrumentContract>>& trades,
                                     std::vector<std::shared_ptr<Contract>>& structures);

        std::string validationRoot() const override;

        bool portfolioIsMargined;
        bool prepared = false;

        std::shared_ptr<Portfolio> portfolio;
        std::vector<std::shared_ptr<Contract>> nettingSets;

        std::shared_ptr<GbmPathGenerator> pathGenerator;
        std::shared_ptr<QuantLib::Calendar> calendarForHoldingPeriod;
        size_t holdingPeriod;

        std::vector<QuantLib::Date> simulationDates;
        std::vector<size_t>
            holdingPeriodIndex; // those simulation dates needed only for holding period calculations
        std::vector<size_t> exposureDatesIndex; // dates on which exposures will be calculated
        size_t simulationSteps;
        size_t exposureSteps;
    };

}
