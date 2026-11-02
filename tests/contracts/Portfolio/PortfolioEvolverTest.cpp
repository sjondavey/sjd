#include "PortfolioEvolverTest.h"

#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFwdVol.h>

using namespace std;
using namespace QuantLib;
using namespace boost::unit_test_framework;
using namespace sjd;

namespace
{

    const double valueTolerance = 1e-6;

    std::shared_ptr<RatesEnvironment> europeanMarket(GenerateRates& gr,
                                                     Date anchor,
                                                     const vector<Date>& manditoryDates)
    {
        std::shared_ptr<RatesEnvironment> re =
            gr.getRatesEnvironmentForEuropeanOption(anchor, manditoryDates);
        std::shared_ptr<ForwardCurveDeterministic> deterministic =
            std::dynamic_pointer_cast<ForwardCurveDeterministic>(re->getFRS());
        BOOST_REQUIRE(deterministic);
        std::shared_ptr<ForwardCurveStochasticFwdVol> stochastic(
            std::make_shared<ForwardCurveStochasticFwdVol>(
                anchor, deterministic->getObservationDates(), deterministic->getForwards(),
                ArrayInterpolatorType::LINEAR, false, re->getVRS()));
        re->setFRS(stochastic);
        BOOST_REQUIRE_MESSAGE(re->isOK(), re->getErrorMessagesAsString());
        return re;
    }

    std::shared_ptr<EuropeanOption> soldPut(Date anchor, double strike)
    {
        return GenerateContracts::getDefaultEuropeanOption(anchor, 2, strike, 10000.0, BuySell::SELL,
                                                           PutCall::PUT, 0.0, false);
    }

    std::shared_ptr<PortfolioEvolver> evolverFor(std::shared_ptr<Portfolio> portfolio,
                                                 std::shared_ptr<RatesEnvironment> re,
                                                 const set<Date>& simulationDates)
    {
        std::shared_ptr<SingleFactorEconomicScenarioGenerator> generator(
            std::make_shared<SingleFactorEconomicScenarioGenerator>(re));
        return std::make_shared<PortfolioEvolver>(portfolio, generator, simulationDates);
    }

    vector<double> markToMarketAlong(std::shared_ptr<Portfolio> portfolio, std::shared_ptr<Path> path)
    {
        vector<double> mtm(path->getNumberOfSteps());
        for (size_t i = 0; i < mtm.size(); ++i)
        {
            mtm[i] = portfolio->getValue(path->getRatesEnvironment(i));
        }
        return mtm;
    }

}

void PortfolioEvolverTest::testPortfolioEvovler()
{
    BOOST_TEST_MESSAGE("Testing PortfolioEvolver ...");

    Date anchor(27, Jan, 2013);
    GenerateRates gr;
    Date oneYear = gr.defaultCalendar->advance(anchor, 1, Years);
    std::shared_ptr<EuropeanOption> calendarOption = soldPut(anchor, 100.0);

    vector<Date> manditoryDates;
    manditoryDates.push_back(oneYear);
    manditoryDates.push_back(calendarOption->getSettlementDate());

    std::shared_ptr<RatesEnvironment> re = europeanMarket(gr, anchor, manditoryDates);
    double strike = re->getFRS()->getForward(calendarOption->getSettlementDate());

    set<Date> simulationDates;
    simulationDates.insert(anchor);
    simulationDates.insert(oneYear);

    vector<double> forwardSpots;
    forwardSpots.push_back(re->getFRS()->getForward(anchor));
    forwardSpots.push_back(re->getFRS()->getForward(oneYear));
    vector<double> shockedSpots = forwardSpots;
    shockedSpots[1] = forwardSpots[1] * 0.5;

    std::shared_ptr<EuropeanOption> option = soldPut(anchor, strike);
    vector<std::shared_ptr<Contract>> contracts;
    contracts.push_back(option);
    std::shared_ptr<Portfolio> portfolio(std::make_shared<Portfolio>(contracts));
    std::shared_ptr<PortfolioEvolver> evolver = evolverFor(portfolio, re, simulationDates);
    BOOST_REQUIRE_MESSAGE(evolver->isOK(), evolver->getErrorMessagesAsString());

    SimulatedPaths forwardPath = evolver->valueSpotPath(forwardSpots, false);
    BOOST_REQUIRE_EQUAL(forwardPath.paths, 1u);
    BOOST_REQUIRE_EQUAL(forwardPath.steps, simulationDates.size());
    BOOST_REQUIRE_EQUAL(forwardPath.dates.front(), anchor);

    double anchorValue = portfolio->getValue(re);
    BOOST_CHECK(abs(forwardPath.mtm[0] - anchorValue) < valueTolerance);

    std::shared_ptr<RatesEnvironment> shockedMarket = europeanMarket(gr, anchor, manditoryDates);
    std::shared_ptr<EuropeanOption> shockedOption = soldPut(anchor, strike);
    vector<std::shared_ptr<Contract>> shockedContracts;
    shockedContracts.push_back(shockedOption);
    std::shared_ptr<Portfolio> shockedPortfolio(std::make_shared<Portfolio>(shockedContracts));
    std::shared_ptr<PortfolioEvolver> shockedEvolver =
        evolverFor(shockedPortfolio, shockedMarket, simulationDates);
    BOOST_REQUIRE_MESSAGE(shockedEvolver->isOK(), shockedEvolver->getErrorMessagesAsString());
    SimulatedPaths shockedPath = shockedEvolver->valueSpotPath(shockedSpots, false);
    BOOST_REQUIRE_EQUAL(shockedPath.steps, forwardPath.steps);
    BOOST_CHECK(shockedPath.mtm[1] < forwardPath.mtm[1]);

    bool useMyThreshold = true;
    double myThreshold = 1000.0;
    bool useCptyThreshold = true;
    double cptyThreshold = 1000.0;
    double minimumTransferAmount = 500.0;
    double rounding = 100.0;
    std::shared_ptr<Margin> margin(std::make_shared<SimpleMargin>(anchor, useMyThreshold, myThreshold,
                                                                  useCptyThreshold, cptyThreshold,
                                                                  minimumTransferAmount, rounding));
    std::shared_ptr<RatesEnvironment> marginMarket = europeanMarket(gr, anchor, manditoryDates);
    std::shared_ptr<EuropeanOption> marginOption = soldPut(anchor, strike);
    vector<std::shared_ptr<Contract>> marginContracts;
    marginContracts.push_back(marginOption);
    std::shared_ptr<Portfolio> marginedPortfolio(std::make_shared<Portfolio>(marginContracts, margin));
    std::shared_ptr<PortfolioEvolver> marginEvolver =
        evolverFor(marginedPortfolio, marginMarket, simulationDates);
    BOOST_REQUIRE_MESSAGE(marginEvolver->isOK(), marginEvolver->getErrorMessagesAsString());
    SimulatedPaths marginPath = marginEvolver->valueSpotPath(forwardSpots, false);
    BOOST_REQUIRE_EQUAL(marginPath.margin.size(), marginPath.mtm.size());
    vector<double> mtmPath = marginPath.mtm;
    std::shared_ptr<SimpleMargin> agreement(
        std::make_shared<SimpleMargin>(anchor, useMyThreshold, myThreshold, useCptyThreshold, cptyThreshold,
                                       minimumTransferAmount, rounding));
    vector<double> expectedMargin;
    agreement->getFutureMarginBalancesAlongPath(mtmPath, expectedMargin);
    BOOST_REQUIRE_EQUAL(expectedMargin.size(), mtmPath.size());
    for (size_t i = 0; i < expectedMargin.size(); ++i)
    {
        BOOST_CHECK(abs(marginPath.margin[i] - expectedMargin[i]) < valueTolerance);
    }

    std::shared_ptr<RatesEnvironment> monteCarloMarket = europeanMarket(gr, anchor, manditoryDates);
    std::shared_ptr<EuropeanOption> monteCarloOption = soldPut(anchor, strike);
    vector<std::shared_ptr<Contract>> monteCarloContracts;
    monteCarloContracts.push_back(monteCarloOption);
    std::shared_ptr<Portfolio> monteCarloPortfolio(std::make_shared<Portfolio>(monteCarloContracts));
    std::shared_ptr<SingleFactorEconomicScenarioGenerator> monteCarloGenerator(
        std::make_shared<SingleFactorEconomicScenarioGenerator>(monteCarloMarket));
    std::shared_ptr<PortfolioEvolver> monteCarloEvolver(
        std::make_shared<PortfolioEvolver>(monteCarloPortfolio, monteCarloGenerator, simulationDates));
    BOOST_REQUIRE_MESSAGE(monteCarloEvolver->isOK(), monteCarloEvolver->getErrorMessagesAsString());
    // isOK() only checks the portfolio and the generator. Sampling a path needs prepare(),
    // which is what sets the generator's simulation dates.
    BOOST_REQUIRE_MESSAGE(monteCarloEvolver->prepare(), monteCarloEvolver->getErrorMessagesAsString());

    const long seed = 17;
    monteCarloGenerator->setRandomSeed(seed);
    BOOST_CHECK_EQUAL(monteCarloGenerator->getRandomSeed(), seed);
    vector<vector<double>> sampledMtM;
    sampledMtM.push_back(markToMarketAlong(monteCarloPortfolio, monteCarloGenerator->getRandomPath()));
    sampledMtM.push_back(markToMarketAlong(monteCarloPortfolio, monteCarloGenerator->getAntitheticPath()));

    monteCarloGenerator->setRandomSeed(seed);
    SimulatedPaths simulated = monteCarloEvolver->simulatePaths(1, true, true);
    BOOST_REQUIRE_EQUAL(simulated.paths, 2u);
    BOOST_REQUIRE_EQUAL(simulated.steps, simulationDates.size());
    BOOST_REQUIRE_EQUAL(simulated.trades, 1u);
    BOOST_CHECK(simulated.antithetic);
    BOOST_CHECK(simulated.margin.empty());
    for (size_t p = 0; p < simulated.paths; ++p)
    {
        for (size_t i = 0; i < simulated.steps; ++i)
        {
            size_t cell = p * simulated.steps + i;
            BOOST_CHECK(abs(simulated.mtm[cell] - sampledMtM[p][i]) < valueTolerance);
            BOOST_CHECK(abs(simulated.byTrade[cell] - simulated.mtm[cell]) < valueTolerance);
        }
    }

    monteCarloGenerator->setRandomSeed(seed);
    SimulatedPaths independent = monteCarloEvolver->simulatePaths(3, false, false);
    BOOST_REQUIRE_EQUAL(independent.paths, 3u);
    BOOST_CHECK(independent.byTrade.empty());
    for (size_t i = 0; i < independent.steps; ++i)
    {
        BOOST_CHECK(abs(independent.mtm[i] - sampledMtM[0][i]) < valueTolerance);
    }

    SimulatedPaths none = monteCarloEvolver->simulatePaths(0, true, false);
    BOOST_CHECK_EQUAL(none.paths, 0u);
    BOOST_CHECK(monteCarloEvolver->getErrorMessagesAsString().find("Number of paths") != string::npos);
}

test_suite* PortfolioEvolverTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("PortfolioEvolver");
    suite->add(BOOST_TEST_CASE(&PortfolioEvolverTest::testPortfolioEvovler));
    return suite;
}
