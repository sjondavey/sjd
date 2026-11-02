#include <sjd/contracts/Portfolio/PortfolioEvolver.h>

#include <sstream>

#include <boost/lexical_cast.hpp>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    PortfolioEvolver::PortfolioEvolver(
        std::shared_ptr<Portfolio> portfolioInput,
        std::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGeneratorInput,
        set<Date> forwardDatesSet)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("PortfolioEvolver"));
        portfolio = portfolioInput;
        portfolioIsMargined = portfolio->isMargined();
        pathGenerator = pathGeneratorInput;
        holdingPeriod = 0;
        setSimulationDates(forwardDatesSet);
    }

    PortfolioEvolver::PortfolioEvolver(
        std::shared_ptr<Portfolio> portfolioInput,
        std::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGeneratorInput,
        set<Date> forwardDatesSet,
        std::shared_ptr<Calendar> calendarForHoldingPeriodInput,
        size_t holdingPeriodInput)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("PortfolioEvolver"));
        portfolio = portfolioInput;
        portfolioIsMargined = portfolio->isMargined();
        pathGenerator = pathGeneratorInput;
        holdingPeriod = holdingPeriodInput;
        calendarForHoldingPeriod = calendarForHoldingPeriodInput;
        setSimulationDates(forwardDatesSet);
    }

    std::string PortfolioEvolver::validationRoot() const
    {
        return "portfolio_evolver";
    }

    ValidationReport PortfolioEvolver::validateReport()
    {
        isOK();
        return validationReport();
    }

    bool PortfolioEvolver::isOK()
    {
        errorTracking->clear();
        if ((holdingPeriod > 0) && !calendarForHoldingPeriod)
        {
            recordIssue("HOLDING_PERIOD_CALENDAR_MISSING",
                        "A holding period needs a calendar to count its business days", {"holding_period"});
            return false;
        }
        if (simulationDates.size() == 0)
        {
            recordIssue("SIMULATION_DATES_EMPTY", "Simulation Dates are empty", {"simulation_dates"});
            return false;
        }
        if (simulationDates.front() != pathGenerator->getAnchorDate())
        {
            recordIssue("SIMULATION_ANCHOR_MISMATCH", "First simulation date must be the anchor date",
                        {"simulation_dates"});
            return false;
        }
        portfolio->validate();
        if (!portfolio->isOK())
        {
            absorbChildReport(portfolio->validationReport(), {});
            return false;
        }
        set<Date> valuationDatesSet = set<Date>(simulationDates.begin(), simulationDates.end());
        pathGenerator->setSimulationDates(valuationDatesSet);
        if (!pathGenerator->isOK())
        {
            absorbChildReport(pathGenerator->validationReport(), {});
            return false;
        }
        std::shared_ptr<Path> testPath = pathGenerator->getConfidenceInterval(0.95);
        for (size_t i = 0; i < testPath->getNumberOfSteps(); ++i)
        {
            portfolio->validate(testPath->getRatesEnvironment(i));
            if (!portfolio->isOK())
            {
                stringstream descriptionStream;
                descriptionStream << "Portfolio cannot value against Rate Environment ";
                descriptionStream << i << ": " << portfolio->getErrorMessagesAsString();
                recordIssue("SIMULATION_MARKET_INVALID", descriptionStream.str(),
                            {"simulation_steps", boost::lexical_cast<std::string>(i)});
                absorbChildReport(portfolio->validationReport(),
                                  {"simulation_steps", boost::lexical_cast<std::string>(i)});
                return false;
            }
        }
        return !errorTracking->getHasErrors();
    }

    vector<Date> PortfolioEvolver::getSimulationDates()
    {
        return simulationDates;
    }

    vector<Date> PortfolioEvolver::getExposureDates()
    {
        vector<Date> subset = vector<Date>(exposureSteps);
        for (size_t i = 0; i < exposureSteps; ++i)
        {
            subset[i] = simulationDates[exposureDatesIndex[i]];
        }
        return subset;
    }

    SimulatedPaths PortfolioEvolver::prepareResult(size_t paths,
                                                   bool antithetic,
                                                   bool byTrade,
                                                   bool byStructure,
                                                   vector<std::shared_ptr<SingleInstrumentContract>>& trades,
                                                   vector<std::shared_ptr<Contract>>& structures)
    {
        SimulatedPaths result;
        if (byTrade)
        {
            trades = portfolio->getUnderlyingSingleInstrumentContracts();
        }
        if (byStructure)
        {
            structures = portfolio->regulatoryStructures();
        }
        result.dates = simulationDates;
        result.exposureSteps = exposureDatesIndex;
        result.marginResetSteps = holdingPeriodIndex;
        result.paths = paths;
        result.steps = simulationSteps;
        result.trades = trades.size();
        result.structures = structures.size();
        result.antithetic = antithetic;
        result.spot.assign(paths * simulationSteps, 0.0);
        result.mtm.assign(paths * simulationSteps, 0.0);
        if (portfolioIsMargined)
        {
            result.margin.assign(paths * simulationSteps, 0.0);
        }
        if (!trades.empty())
        {
            result.byTrade.assign(paths * simulationSteps * trades.size(), 0.0);
        }
        if (!structures.empty())
        {
            result.byStructure.assign(paths * simulationSteps * structures.size(), 0.0);
        }
        return result;
    }

    SimulatedPaths PortfolioEvolver::simulatePaths(unsigned long N,
                                                   bool antithetic,
                                                   bool byTrade,
                                                   bool byStructure)
    {
        SimulatedPaths result;
        if (!validateReport().isOK())
        {
            return result;
        }
        if (N == 0)
        {
            recordIssue("MONTE_CARLO_NO_PATHS", "Number of paths must be at least 1", {"number_of_paths"});
            return result;
        }
        set<Date> valuationDatesSet = set<Date>(simulationDates.begin(), simulationDates.end());
        if (!pathGenerator->setSimulationDates(valuationDatesSet) || (!pathGenerator->isOK()))
        {
            return result;
        }
        if (antithetic)
        {
            N += N % 2;
        }
        vector<std::shared_ptr<SingleInstrumentContract>> trades;
        vector<std::shared_ptr<Contract>> structures;
        result = prepareResult(N, antithetic, byTrade, byStructure, trades, structures);

        size_t draws = antithetic ? N / 2 : N;
        for (size_t i = 0; i < draws; ++i)
        {
            recordPath(result, i, pathGenerator->getRandomPath(), trades, structures);
            if (antithetic)
            {
                recordPath(result, draws + i, pathGenerator->getAntitheticPath(), trades, structures);
            }
        }
        return result;
    }

    SimulatedPaths PortfolioEvolver::valueSpotPath(const vector<double>& spots,
                                                   bool byTrade,
                                                   bool byStructure)
    {
        return valueSpotPaths(spots, 1, byTrade, byStructure);
    }

    SimulatedPaths PortfolioEvolver::valueSpotPaths(const vector<double>& spots,
                                                    size_t numberOfPaths,
                                                    bool byTrade,
                                                    bool byStructure)
    {
        SimulatedPaths result;
        if (!validateReport().isOK())
        {
            return result;
        }
        if (numberOfPaths == 0)
        {
            recordIssue("SPOT_PATHS_EMPTY", "Number of spot paths must be at least 1", {"spot_paths"});
            return result;
        }
        if (spots.size() != numberOfPaths * simulationSteps)
        {
            recordIssue("SPOT_PATHS_LENGTH", "Spot paths must be number_of_paths times simulation dates",
                        {"spot_paths"});
            return result;
        }
        vector<std::shared_ptr<SingleInstrumentContract>> trades;
        vector<std::shared_ptr<Contract>> structures;
        result = prepareResult(numberOfPaths, false, byTrade, byStructure, trades, structures);
        vector<double> pathSpots(simulationSteps);
        for (size_t i = 0; i < numberOfPaths; ++i)
        {
            copy(spots.begin() + static_cast<std::ptrdiff_t>(i * simulationSteps),
                 spots.begin() + static_cast<std::ptrdiff_t>((i + 1) * simulationSteps), pathSpots.begin());
            recordPath(result, i, pathGenerator->getPathForSpot(pathSpots), trades, structures);
        }
        return result;
    }

    void PortfolioEvolver::recordPath(SimulatedPaths& result,
                                      size_t row,
                                      std::shared_ptr<Path> path,
                                      const vector<std::shared_ptr<SingleInstrumentContract>>& trades,
                                      const vector<std::shared_ptr<Contract>>& structures)
    {
        evolvePortfolioAlongPath(path);
        size_t offset = row * result.steps;
        for (size_t j = 0; j < result.steps; ++j)
        {
            std::shared_ptr<RatesEnvironment> environment = path->getRatesEnvironment(j);
            result.spot[offset + j] = spotPath[j];
            result.mtm[offset + j] = mtmOnPath[j];
            if (portfolioIsMargined)
            {
                result.margin[offset + j] = marginOnPath[j];
            }
            for (size_t k = 0; k < trades.size(); ++k)
            {
                result.byTrade[(offset + j) * trades.size() + k] = trades[k]->getValue(environment);
            }
            for (size_t k = 0; k < structures.size(); ++k)
            {
                result.byStructure[(offset + j) * structures.size() + k] =
                    structures[k]->getValue(environment);
            }
        }
    }

    void PortfolioEvolver::setSimulationDates(set<Date> dates)
    {
        holdingPeriodIndex.clear();
        exposureDatesIndex.clear();
        simulationSteps = 0;
        exposureSteps = 0;
        simulationDates = vector<Date>(dates.begin(), dates.end());

        if ((holdingPeriod == 0) || !portfolioIsMargined || !calendarForHoldingPeriod)
        {
            for (size_t i = 0; i < simulationDates.size(); ++i)
            {
                exposureDatesIndex.push_back(i);
                holdingPeriodIndex.push_back(i);
            }
        }
        else
        {
            set<Date> exposureDatesSet = set<Date>(dates.begin(), dates.end());
            set<Date> merged = exposureDatesSet;
            vector<Date> regCapMarginResetDates;
            int numDays = holdingPeriod;
            set<Date>::iterator it;
            for (it = exposureDatesSet.begin(); it != exposureDatesSet.end(); ++it)
            {
                Date marginResetDate = calendarForHoldingPeriod->advance(*it, -numDays, Days);
                if (marginResetDate < portfolio->getMargin()->getAnchorDate())
                {
                    marginResetDate = portfolio->getMargin()->getAnchorDate();
                }
                regCapMarginResetDates.push_back(marginResetDate);
                merged.insert(marginResetDate);
            }

            simulationDates = vector<Date>(merged.begin(), merged.end());

            vector<Date> exposureDates = vector<Date>(exposureDatesSet.begin(), exposureDatesSet.end());

            exposureDatesIndex.clear();
            for (size_t i = 0; i < exposureDates.size(); ++i)
            {
                vector<Date>::iterator found;
                found = find(simulationDates.begin(), simulationDates.end(), exposureDates[i]);
                size_t index = std::distance(simulationDates.begin(), found);
                exposureDatesIndex.push_back(index);
            }
            holdingPeriodIndex.clear();
            for (size_t i = 0; i < regCapMarginResetDates.size(); ++i)
            {
                vector<Date>::iterator found;
                found = find(simulationDates.begin(), simulationDates.end(), regCapMarginResetDates[i]);
                size_t index = std::distance(simulationDates.begin(), found);
                holdingPeriodIndex.push_back(index);
            }
        }
        simulationSteps = simulationDates.size();
        exposureSteps = exposureDatesIndex.size();
    }

    vector<double> PortfolioEvolver::getSpotAlongAPath(std::shared_ptr<Path> inputPath)
    {
        size_t steps = inputPath->getNumberOfSteps();
        vector<double> spot = vector<double>(steps);
        for (size_t i = 0; i < steps; ++i)
        {
            Date ad = inputPath->getRatesEnvironment(i)->getAnchorDate();
            spot[i] = inputPath->getRatesEnvironment(i)->getFRS()->getForward(ad);
        }
        return spot;
    }

    vector<double> PortfolioEvolver::getMtMAlongAPath(std::shared_ptr<Path> inputPath)
    {
        size_t steps = inputPath->getNumberOfSteps();
        vector<double> mtm = vector<double>(steps);
        for (size_t i = 0; i < steps; ++i)
        {
            mtm[i] = portfolio->getValue(inputPath->getRatesEnvironment(i));
        }
        return mtm;
    }

    void PortfolioEvolver::evolvePortfolioAlongPath(std::shared_ptr<Path> inputPath)
    {
        spotPath = getSpotAlongAPath(inputPath);
        mtmOnPath = getMtMAlongAPath(inputPath);
        if (portfolioIsMargined)
        {
            portfolio->getMargin()->getFutureMarginBalancesAlongPath(mtmOnPath, marginOnPath);
        }
    }

}
