#include <benchmark/benchmark.h>

#include <cmath>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

#include <sjd/contracts/Portfolio/PortfolioEvolver.h>
#include <sjd/pricing/AsianOptionFormula.h>
#include <sjd/pricing/Black76Formula.h>
#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFwdVol.h>
#include <support/GenerateContracts.h>
#include <support/GenerateRatesEnvironments.h>

using namespace QuantLib;
using namespace sjd;

namespace
{

    std::shared_ptr<PortfolioEvolver> oneOptionBook()
    {
        Date anchor(27, Jan, 2013);
        GenerateRates gr;
        Date oneYear = gr.defaultCalendar.advance(anchor, 1, Years);
        std::shared_ptr<EuropeanOption> option = GenerateContracts::getDefaultEuropeanOption(
            anchor, 2, 100.0, 10000.0, BuySell::SELL, PutCall::PUT, 0.0, false);
        std::vector<Date> mandatory;
        mandatory.push_back(oneYear);
        mandatory.push_back(option->getSettlementDate());
        std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForEuropeanOption(anchor, mandatory);
        std::shared_ptr<ForwardCurveDeterministic> deterministic =
            std::dynamic_pointer_cast<ForwardCurveDeterministic>(re->getFRS());
        if (!deterministic)
        {
            throw std::runtime_error("benchmark book has no deterministic forward");
        }
        std::shared_ptr<ForwardCurveStochasticFwdVol> stochastic =
            std::make_shared<ForwardCurveStochasticFwdVol>(
                anchor, deterministic->getObservationDates(), deterministic->getForwards(),
                ArrayInterpolatorType::LINEAR, false, re->getVRS());
        re->setFRS(stochastic);
        std::vector<std::shared_ptr<Contract>> contracts;
        contracts.push_back(option);
        std::shared_ptr<Portfolio> portfolio = std::make_shared<Portfolio>(contracts);
        std::set<Date> dates;
        dates.insert(anchor);
        dates.insert(oneYear);
        std::shared_ptr<SingleFactorEconomicScenarioGenerator> generator =
            std::make_shared<SingleFactorEconomicScenarioGenerator>(re);
        std::shared_ptr<PortfolioEvolver> evolver =
            std::make_shared<PortfolioEvolver>(portfolio, generator, dates);
        if (!evolver->validate().isOK() || !evolver->prepare())
        {
            throw std::runtime_error(evolver->getErrorMessagesAsString());
        }
        return evolver;
    }

}

static void black76(benchmark::State& state)
{
    Black76Call call(100.0, 110.0, 0.2, 0.97);
    for (auto _ : state)
    {
        (void)_;
        benchmark::DoNotOptimize(call.getPremium());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(black76);

static void asian(benchmark::State& state)
{
    std::vector<double> fwds;
    std::vector<double> sds;
    const double spot = 21.13;
    const double rfr = 0.065;
    const double yield = 0.025;
    const double vol = 0.25;
    for (int i = 366; i < 386; ++i)
    {
        const double yearsAhead = static_cast<double>(i) / 365.0;
        fwds.push_back(spot * std::exp(-(rfr - yield) * (yearsAhead + 2.0 / 365.0)));
        sds.push_back(vol * std::sqrt(yearsAhead));
    }
    const std::vector<double> fixes;
    const double df = std::exp(-rfr * (386.0 + 2.0) / 365.0);
    for (auto _ : state)
    {
        (void)_;
        AverageRateCall call(fwds, fixes, 22.0, sds, df);
        benchmark::DoNotOptimize(call.getPremium());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(asian);

static void onePath(benchmark::State& state)
{
    std::shared_ptr<PortfolioEvolver> evolver = oneOptionBook();
    for (auto _ : state)
    {
        (void)_;
        SimulatedPaths paths = evolver->simulatePaths(1, false, false);
        benchmark::DoNotOptimize(paths.mtm.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(onePath);

static void tenThousandPaths(benchmark::State& state)
{
    std::shared_ptr<PortfolioEvolver> evolver = oneOptionBook();
    for (auto _ : state)
    {
        (void)_;
        SimulatedPaths paths = evolver->simulatePaths(10000, false, false);
        benchmark::DoNotOptimize(paths.mtm.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(tenThousandPaths)->Iterations(1)->Repetitions(1);

BENCHMARK_MAIN();
