#include <sjd/contracts/Portfolio/PortfolioEvolver.h>

#include <sjd/contracts/Portfolio/Structures/CemNettingSet.h>
#include <sjd/tools/Errors.h>
#include <sjd/tools/Log.h>

#include <ql/math/distributions/normaldistribution.hpp>
#include <ql/math/randomnumbers/sobolrsg.hpp>
#include <ql/methods/montecarlo/brownianbridge.hpp>

#include <cmath>
#include <exception>
#include <memory>
#include <sstream>
#include <thread>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    namespace
    {
        size_t workerCount(size_t requested, size_t work)
        {
            size_t count = requested == 0 ? std::thread::hardware_concurrency() : requested;
            if (count == 0)
            {
                count = 1;
            }
            if (work > 0 && count > work)
            {
                count = work;
            }
            return count;
        }

        double linearQuantile(std::vector<double> values, double level)
        {
            if (values.empty())
            {
                return 0.0;
            }
            std::sort(values.begin(), values.end());
            const double position = level * static_cast<double>(values.size() - 1);
            const size_t lower = static_cast<size_t>(std::floor(position));
            const size_t upper = static_cast<size_t>(std::ceil(position));
            const double weight = position - static_cast<double>(lower);
            return values[lower] * (1.0 - weight) + values[upper] * weight;
        }

        void fillAggregates(SimulatedPaths& result, const std::vector<double>& capitalSum)
        {
            const size_t paths = result.paths;
            const size_t steps = result.steps;
            result.meanSpot.assign(steps, 0.0);
            result.meanMtm.assign(steps, 0.0);
            result.expectedExposure.assign(steps, 0.0);
            result.potentialFutureExposure.assign(steps, 0.0);
            if (!result.margin.empty())
            {
                result.meanMargin.assign(steps, 0.0);
            }
            for (size_t i = 0; i < paths; ++i)
            {
                for (size_t j = 0; j < steps; ++j)
                {
                    const double spot = result.spot[i * steps + j];
                    const double mtm = result.mtm[i * steps + j];
                    result.meanSpot[j] += spot;
                    result.meanMtm[j] += mtm;
                    result.expectedExposure[j] += std::max(mtm, 0.0);
                    if (!result.meanMargin.empty())
                    {
                        result.meanMargin[j] += result.margin[i * steps + j];
                    }
                }
            }
            std::vector<double> column(paths);
            for (size_t j = 0; j < steps; ++j)
            {
                result.meanSpot[j] /= static_cast<double>(paths);
                result.meanMtm[j] /= static_cast<double>(paths);
                result.expectedExposure[j] /= static_cast<double>(paths);
                if (!result.meanMargin.empty())
                {
                    result.meanMargin[j] /= static_cast<double>(paths);
                }
                for (size_t i = 0; i < paths; ++i)
                {
                    column[i] = std::max(result.mtm[i * steps + j], 0.0);
                }
                result.potentialFutureExposure[j] = linearQuantile(column, 0.95);
            }
            if (!capitalSum.empty())
            {
                result.meanCapital.resize(capitalSum.size());
                for (size_t e = 0; e < capitalSum.size(); ++e)
                {
                    result.meanCapital[e] = capitalSum[e] / static_cast<double>(paths);
                }
            }
            result.aggregated = true;
            result.spot.clear();
            result.mtm.clear();
            result.margin.clear();
            result.byTrade.clear();
            result.byStructure.clear();
        }

        std::vector<QuantLib::Time> bridgeTimes(const std::vector<QuantLib::Date>& dates)
        {
            std::vector<QuantLib::Time> times(dates.size());
            double previous = 0.0;
            const QuantLib::Date anchor = dates.front();
            for (size_t i = 0; i < dates.size(); ++i)
            {
                double year = static_cast<double>(dates[i] - anchor) / 365.0;
                if (year <= previous)
                {
                    year = previous + 1e-8;
                }
                times[i] = year;
                previous = year;
            }
            return times;
        }

        // Sobol points inverted to normals, then a Brownian bridge. QuantLib's bridge
        // already divides each increment by the square root of the step, so the shocks
        // are unit normals and the early dimensions set the coarse path.
        struct QuasiPath
        {
            QuantLib::SobolRsg sobol;
            QuantLib::InverseCumulativeNormal inverse;
            QuantLib::BrownianBridge bridge;
            std::vector<double> gaussian;
            std::vector<double> shocks;

            QuasiPath(size_t steps, unsigned long seed, const std::vector<QuantLib::Time>& times)
                : sobol(steps, seed), inverse(), bridge(times), gaussian(steps), shocks(steps)
            {
            }

            void skip(size_t count)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    sobol.nextSequence();
                }
            }

            const std::vector<double>& next()
            {
                const QuantLib::SobolRsg::sample_type& sample = sobol.nextSequence();
                for (size_t k = 0; k < gaussian.size(); ++k)
                {
                    double unit = sample.value[k];
                    if (unit < 1e-12)
                    {
                        unit = 1e-12;
                    }
                    if (unit > 1.0 - 1e-12)
                    {
                        unit = 1.0 - 1e-12;
                    }
                    gaussian[k] = inverse(unit);
                }
                bridge.transform(gaussian.begin(), gaussian.end(), shocks.begin());
                return shocks;
            }
        };
    }

    PortfolioEvolver::PortfolioEvolver(
        std::shared_ptr<Portfolio> portfolioInput,
        std::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGeneratorInput,
        set<Date> forwardDatesSet)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("PortfolioEvolver");
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
        const Calendar& calendarForHoldingPeriodInput,
        size_t holdingPeriodInput)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("PortfolioEvolver");
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

    void PortfolioEvolver::runChecks()
    {
        errorTracking->clear();
        if ((holdingPeriod > 0) && calendarForHoldingPeriod.empty())
        {
            recordIssue(issues::HOLDING_PERIOD_CALENDAR_MISSING,
                        "A holding period needs a calendar to count its business days", {"holding_period"});
            return;
        }
        if (simulationDates.size() == 0)
        {
            recordIssue(issues::SIMULATION_DATES_EMPTY, "Simulation Dates are empty", {"simulation_dates"});
            return;
        }
        if (simulationDates.front() != pathGenerator->getAnchorDate())
        {
            recordIssue(issues::SIMULATION_ANCHOR_MISMATCH, "First simulation date must be the anchor date",
                        {"simulation_dates"});
            return;
        }
        portfolio->validate();
        if (!portfolio->isOK())
        {
            absorbChildReport(portfolio->validationReport(), {});
            return;
        }
        if (!pathGenerator->validate().isOK())
        {
            absorbChildReport(pathGenerator->validationReport(), {});
            return;
        }
        return;
    }

    bool PortfolioEvolver::prepare()
    {
        prepared = false;
        if (!validate().isOK())
        {
            return false;
        }
        set<Date> valuationDatesSet(simulationDates.begin(), simulationDates.end());
        if (!pathGenerator->setSimulationDates(valuationDatesSet) || !pathGenerator->validate().isOK())
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
                recordIssue(issues::SIMULATION_MARKET_INVALID, descriptionStream.str(),
                            {"simulation_steps", std::to_string(i)});
                absorbChildReport(portfolio->validationReport(), {"simulation_steps", std::to_string(i)});
                return false;
            }
        }
        prepared = !errorTracking->getHasErrors();
        return prepared;
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

    void PortfolioEvolver::setNettingSets(vector<std::shared_ptr<Contract>> sets)
    {
        nettingSets = std::move(sets);
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
            structures = nettingSets;
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
        SimulationOptions options;
        options.antithetic = antithetic;
        options.byTrade = byTrade;
        options.byNettingSet = byStructure;
        options.threads = 1;
        return simulatePaths(N, options);
    }

    SimulatedPaths PortfolioEvolver::simulatePaths(unsigned long N, const SimulationOptions& options)
    {
        SJD_ASSERT(prepared);
        SimulatedPaths result;
        if (!prepared)
        {
            return result;
        }
        if (N == 0)
        {
            recordIssue(issues::MONTE_CARLO_NO_PATHS, "Number of paths must be at least 1",
                        {"number_of_paths"});
            return result;
        }
        if (options.antithetic)
        {
            N += N % 2;
        }
        const bool storeTrades = options.byTrade && !options.aggregates;
        const bool storeSets = options.byNettingSet && !options.aggregates;
        vector<std::shared_ptr<SingleInstrumentContract>> trades;
        vector<std::shared_ptr<Contract>> structures;
        result = prepareResult(N, options.antithetic, storeTrades, storeSets, trades, structures);
        if (options.byNettingSet)
        {
            result.structures = nettingSets.size();
        }

        vector<std::shared_ptr<CemNettingSet>> capitalSets;
        if (options.aggregates && options.capital)
        {
            for (const std::shared_ptr<Contract>& contract : nettingSets)
            {
                std::shared_ptr<CemNettingSet> set = std::dynamic_pointer_cast<CemNettingSet>(contract);
                if (set)
                {
                    capitalSets.push_back(set);
                }
            }
        }

        const size_t draws = options.antithetic ? N / 2 : N;
        const size_t workers = workerCount(options.threads, draws);
        vector<vector<double>> capitalSums(workers);
        vector<std::exception_ptr> errors(workers);
        const vector<QuantLib::Time> times =
            options.sampling == PathSampling::Sobol ? bridgeTimes(simulationDates) : vector<QuantLib::Time>{};

        auto priceDraws = [&](size_t worker, size_t begin, size_t end)
        {
            try
            {
                PathWorkspace space = pathGenerator->workspace();
                vector<double>& capitalSum = capitalSums[worker];
                if (!capitalSets.empty())
                {
                    capitalSum.assign(exposureSteps, 0.0);
                }
                std::unique_ptr<Mt19937Normals> stream;
                std::unique_ptr<QuasiPath> quasi;
                if (options.sampling == PathSampling::Pseudo)
                {
                    stream = std::make_unique<Mt19937Normals>(pathGenerator->getRandomSeed());
                    stream->discardNormals(static_cast<std::uint64_t>(begin) * simulationSteps);
                }
                else
                {
                    quasi = std::make_unique<QuasiPath>(
                        simulationSteps, static_cast<unsigned long>(pathGenerator->getRandomSeed()), times);
                    quasi->skip(begin);
                }
                for (size_t i = begin; i < end; ++i)
                {
                    std::shared_ptr<Path> priced;
                    if (stream)
                    {
                        priced = pathGenerator->generatePath(*stream, space);
                    }
                    else
                    {
                        priced = pathGenerator->writeShocks(quasi->next(), space);
                    }
                    recordPath(result, i, priced, trades, structures);
                    if (sjd::log::logger().enabled(sjd::log::Level::Trace))
                    {
                        SJD_LOG_TRACE("path " + std::to_string(i) + " mtm " +
                                      std::to_string(result.mtm[i * result.steps]));
                    }
                    if (!capitalSets.empty())
                    {
                        addPathCapital(priced, i, result, capitalSets, options.capitalScale, capitalSum);
                    }
                    if (options.antithetic)
                    {
                        priced = pathGenerator->generateAntithetic(space);
                        recordPath(result, draws + i, priced, trades, structures);
                        if (sjd::log::logger().enabled(sjd::log::Level::Trace))
                        {
                            SJD_LOG_TRACE("path " + std::to_string(draws + i) + " mtm " +
                                          std::to_string(result.mtm[(draws + i) * result.steps]));
                        }
                        if (!capitalSets.empty())
                        {
                            addPathCapital(priced, draws + i, result, capitalSets, options.capitalScale,
                                           capitalSum);
                        }
                    }
                }
            }
            catch (...)
            {
                errors[worker] = std::current_exception();
            }
        };

        vector<std::thread> pool;
        const size_t chunk = draws / workers;
        size_t cursor = 0;
        for (size_t worker = 0; worker + 1 < workers; ++worker)
        {
            const size_t stop = cursor + chunk;
            pool.emplace_back(priceDraws, worker, cursor, stop);
            cursor = stop;
        }
        priceDraws(workers - 1, cursor, draws);
        for (std::thread& worker : pool)
        {
            worker.join();
        }
        for (const std::exception_ptr& error : errors)
        {
            if (error)
            {
                std::rethrow_exception(error);
            }
        }

        if (options.aggregates)
        {
            vector<double> capitalSum;
            if (!capitalSets.empty())
            {
                capitalSum.assign(exposureSteps, 0.0);
                for (const vector<double>& part : capitalSums)
                {
                    for (size_t e = 0; e < part.size(); ++e)
                    {
                        capitalSum[e] += part[e];
                    }
                }
            }
            fillAggregates(result, capitalSum);
        }
        return result;
    }

    SimulatedPaths PortfolioEvolver::valueSpotPath(const vector<double>& spots,
                                                   bool byTrade,
                                                   bool byStructure)
    {
        return valueSpotPaths(spots, 1, byTrade, byStructure);
    }

    SimulatedPaths PortfolioEvolver::valueSpotPaths(
        const vector<double>& spots, size_t numberOfPaths, bool byTrade, bool byStructure, size_t threads)
    {
        SJD_ASSERT(prepared);
        SimulatedPaths result;
        if (!prepared)
        {
            return result;
        }
        if (numberOfPaths == 0)
        {
            recordIssue(issues::SPOT_PATHS_EMPTY, "Number of spot paths must be at least 1", {"spot_paths"});
            return result;
        }
        if (spots.size() != numberOfPaths * simulationSteps)
        {
            recordIssue(issues::SPOT_PATHS_LENGTH,
                        "Spot paths must be number_of_paths times simulation dates", {"spot_paths"});
            return result;
        }
        vector<std::shared_ptr<SingleInstrumentContract>> trades;
        vector<std::shared_ptr<Contract>> structures;
        result = prepareResult(numberOfPaths, false, byTrade, byStructure, trades, structures);
        const size_t workers = workerCount(threads, numberOfPaths);
        vector<std::exception_ptr> errors(workers);

        auto priceSpots = [&](size_t worker, size_t begin, size_t end)
        {
            try
            {
                PathWorkspace space = pathGenerator->workspace();
                vector<double> pathSpots(simulationSteps);
                for (size_t i = begin; i < end; ++i)
                {
                    copy(spots.begin() + static_cast<std::ptrdiff_t>(i * simulationSteps),
                         spots.begin() + static_cast<std::ptrdiff_t>((i + 1) * simulationSteps),
                         pathSpots.begin());
                    recordPath(result, i, pathGenerator->generateSpot(pathSpots, space), trades, structures);
                }
            }
            catch (...)
            {
                errors[worker] = std::current_exception();
            }
        };

        vector<std::thread> pool;
        const size_t chunk = numberOfPaths / workers;
        size_t cursor = 0;
        for (size_t worker = 0; worker + 1 < workers; ++worker)
        {
            const size_t stop = cursor + chunk;
            pool.emplace_back(priceSpots, worker, cursor, stop);
            cursor = stop;
        }
        priceSpots(workers - 1, cursor, numberOfPaths);
        for (std::thread& worker : pool)
        {
            worker.join();
        }
        for (const std::exception_ptr& error : errors)
        {
            if (error)
            {
                std::rethrow_exception(error);
            }
        }
        return result;
    }

    void PortfolioEvolver::recordPath(SimulatedPaths& result,
                                      size_t row,
                                      std::shared_ptr<Path> path,
                                      const vector<std::shared_ptr<SingleInstrumentContract>>& trades,
                                      const vector<std::shared_ptr<Contract>>& structures)
    {
        // Price each trade once. The sum is the portfolio value, and by_trade stores
        // those same numbers instead of pricing the book a second time.
        vector<double> spot;
        vector<double> mtm;
        vector<double> margin;
        if (trades.empty())
        {
            evolvePortfolioAlongPath(path, spot, mtm, margin);
        }
        else
        {
            spot = getSpotAlongAPath(path);
            mtm.assign(path->getNumberOfSteps(), 0.0);
            for (size_t j = 0; j < result.steps; ++j)
            {
                std::shared_ptr<RatesEnvironment> environment = path->getRatesEnvironment(j);
                double sum = 0;
                for (size_t k = 0; k < trades.size(); ++k)
                {
                    const double value = trades[k]->getValue(environment);
                    sum += value;
                    result.byTrade[(row * result.steps + j) * trades.size() + k] = value;
                }
                mtm[j] = sum;
            }
            if (portfolioIsMargined)
                portfolio->getMargin()->getFutureMarginBalancesAlongPath(mtm, margin);
        }
        size_t offset = row * result.steps;
        for (size_t j = 0; j < result.steps; ++j)
        {
            std::shared_ptr<RatesEnvironment> environment = path->getRatesEnvironment(j);
            result.spot[offset + j] = spot[j];
            result.mtm[offset + j] = mtm[j];
            if (portfolioIsMargined)
            {
                result.margin[offset + j] = margin[j];
            }
            for (size_t k = 0; k < structures.size(); ++k)
            {
                result.byStructure[(offset + j) * structures.size() + k] =
                    structures[k]->getValue(environment);
            }
        }
    }

    void PortfolioEvolver::addPathCapital(std::shared_ptr<Path> path,
                                          size_t row,
                                          const SimulatedPaths& result,
                                          const vector<std::shared_ptr<CemNettingSet>>& sets,
                                          double scale,
                                          vector<double>& capitalSum) const
    {
        const size_t steps = result.steps;
        const bool margined = portfolioIsMargined;
        vector<double> values(sets.size());
        for (size_t e = 0; e < exposureDatesIndex.size(); ++e)
        {
            const size_t step = exposureDatesIndex[e];
            std::shared_ptr<RatesEnvironment> environment = path->getRatesEnvironment(step);
            double net = 0.0;
            double gross = 0.0;
            for (size_t k = 0; k < sets.size(); ++k)
            {
                values[k] = sets[k]->getValue(environment);
                net += values[k];
                if (values[k] > 0.0)
                {
                    gross += values[k];
                }
            }
            double ratio = 0.0;
            if (net >= 0.0 && gross > 1e-14)
            {
                ratio = net / gross;
            }
            double total = 0.0;
            for (size_t k = 0; k < sets.size(); ++k)
            {
                total += sets[k]->getMtMPlusAddOnForGivenMtM(environment, values[k], ratio);
            }
            double capital = total - net;
            if (margined)
            {
                capital += std::max(net - result.margin[row * steps + step], 0.0);
            }
            else
            {
                capital += std::max(net, 0.0);
            }
            capitalSum[e] += std::max(capital * scale, 0.0);
        }
    }

    void PortfolioEvolver::setSimulationDates(set<Date> dates)
    {
        holdingPeriodIndex.clear();
        exposureDatesIndex.clear();
        simulationSteps = 0;
        exposureSteps = 0;
        simulationDates = vector<Date>(dates.begin(), dates.end());

        if ((holdingPeriod == 0) || !portfolioIsMargined || calendarForHoldingPeriod.empty())
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
            int numDays = static_cast<int>(holdingPeriod);
            set<Date>::iterator it;
            for (it = exposureDatesSet.begin(); it != exposureDatesSet.end(); ++it)
            {
                Date marginResetDate = calendarForHoldingPeriod.advance(*it, -numDays, Days);
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

    void PortfolioEvolver::evolvePortfolioAlongPath(std::shared_ptr<Path> inputPath,
                                                    vector<double>& spot,
                                                    vector<double>& mtm,
                                                    vector<double>& margin)
    {
        spot = getSpotAlongAPath(inputPath);
        mtm = getMtMAlongAPath(inputPath);
        if (portfolioIsMargined)
        {
            portfolio->getMargin()->getFutureMarginBalancesAlongPath(mtm, margin);
        }
    }

}
