#ifndef PORTFOLIOEVOLVER_INCLUDED
#define PORTFOLIOEVOLVER_INCLUDED

#include <algorithm>
#include <set>

#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/rates/PathGenerators/SingleFactorGMB.h>

namespace sjd 
{
    // Every simulated value. Statistics are computed outside the evolver.
    // The arrays are flat and row-major: spot, mtm and margin are [path][step],
    // byTrade is [path][step][trade] and byStructure is [path][step][set].
    // margin is empty unless the portfolio is margined. byTrade and
    // byStructure are empty unless they were requested. A failed
    // simulation has paths == 0. With a holding period, dates also hold the
    // margin reset dates. Exposure date k is step exposureSteps[k], and its
    // margin is read at step marginResetSteps[k].
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
        std::vector<double> spot;
        std::vector<double> mtm;
        std::vector<double> margin;
        std::vector<double> byTrade;
        std::vector<double> byStructure;
    };

    class PortfolioEvolver : public Validatable
    {
    public:
        // Use this for unmargined portfolios or portfolios that do not require holding-period calculations
        PortfolioEvolver(std::shared_ptr<Portfolio> portfolio, 
                         std::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGenerator,
                         std::set<QuantLib::Date> forwardDates);

        // Use this for portfolios that require holding-period calculations
        PortfolioEvolver(std::shared_ptr<Portfolio> portfolio, 
                         std::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGenerator,
                         std::set<QuantLib::Date> forwardDates,
                         std::shared_ptr<QuantLib::Calendar> calendarForHoldingPeriodCalcuation, 
                         size_t holdingPeriodInBusinessDays);

        bool isOK();
        ValidationReport validateReport();

        std::vector<QuantLib::Date> getSimulationDates();
        std::vector<QuantLib::Date> getExposureDates();

        // With antithetic paths, path i and path i + paths/2 use opposite normals, and an
        // odd request is rounded up.
        SimulatedPaths simulatePaths(unsigned long numberOfPaths, bool antithetic, bool byTrade, bool byStructure = false);

        // One path marked to these forwards. spots has one value per simulation date.
        SimulatedPaths valueSpotPath(const std::vector<double>& spots, bool byTrade, bool byStructure = false);

    protected:
        std::vector<double> getSpotAlongAPath(std::shared_ptr<Path> path);
        std::vector<double> getMtMAlongAPath(std::shared_ptr<Path> path);
        virtual void evolvePortfolioAlongPath(std::shared_ptr<Path> path);
        void recordPath(SimulatedPaths& result,
                        size_t row,
                        std::shared_ptr<Path> path,
                        const std::vector<std::shared_ptr<SingleInstrumentContract>>& trades,
                        const std::vector<std::shared_ptr<Contract>>& structures);

        void setSimulationDates(std::set<QuantLib::Date> dates);
        SimulatedPaths prepareResult(size_t paths, bool antithetic, bool byTrade, bool byStructure,
                                     std::vector<std::shared_ptr<SingleInstrumentContract>>& trades,
                                     std::vector<std::shared_ptr<Contract>>& structures);

        std::string validationRoot() const override;

        bool portfolioIsMargined;

        std::shared_ptr<Portfolio> portfolio;

        std::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGenerator;
        std::shared_ptr<QuantLib::Calendar> calendarForHoldingPeriod;
        size_t holdingPeriod;

        std::vector<QuantLib::Date> simulationDates;
        std::vector<size_t> holdingPeriodIndex; // those simulation dates needed only for holding period calculations
        std::vector<size_t> exposureDatesIndex; // dates on which exposures will be calculated
        size_t simulationSteps;
        size_t exposureSteps;

        std::vector<double> spotPath;
        std::vector<double> mtmOnPath;
        std::vector<double> marginOnPath;
    };

}

#endif
