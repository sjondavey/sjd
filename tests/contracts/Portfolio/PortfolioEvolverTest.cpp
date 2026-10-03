#include "PortfolioEvolverTest.h"

#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFwdVol.h>

#include <algorithm>

using namespace std;
using namespace QuantLib;
using namespace boost;
using namespace boost::unit_test_framework;
using namespace sjd;

namespace
{

    const double valueTolerance = 1e-6;

    std::shared_ptr<RatesEnvironment> europeanMarket(GenerateRates& gr,
                                                       Date anchor,
                                                       const vector<Date>& manditoryDates)
    {
        std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForEuropeanOption(anchor, manditoryDates);
        std::shared_ptr<ForwardCurveDeterministic> deterministic =
            std::dynamic_pointer_cast<ForwardCurveDeterministic>(re->getFRS());
        BOOST_REQUIRE(deterministic);
        std::shared_ptr<ForwardCurveStochasticFwdVol> stochastic(
            new ForwardCurveStochasticFwdVol(anchor,
                                             deterministic->getObservationDates(),
                                             deterministic->getForwards(),
                                             LINEAR,
                                             false,
                                             re->getVRS()));
        re->setFRS(stochastic);
        BOOST_REQUIRE_MESSAGE(re->isOK(), re->getErrorMessagesAsString());
        return re;
    }

    std::shared_ptr<EuropeanOption> soldPut(Date anchor, double strike)
    {
        return GenerateContracts::getDefaultEuropeanOption(anchor, 2, strike, 10000.0, SELL, PUT, 0.0, false);
    }

    std::shared_ptr<PortfolioEvolver> evolverFor(std::shared_ptr<Portfolio> portfolio,
                                                   std::shared_ptr<RatesEnvironment> re,
                                                   const set<Date>& simulationDates)
    {
        std::shared_ptr<SingleFactorEconomicScenarioGenerator> generator(
            new SingleFactorEconomicScenarioGenerator(re));
        return std::shared_ptr<PortfolioEvolver>(new PortfolioEvolver(portfolio, generator, simulationDates));
    }

    size_t column(const vector<string>& headings, const string& name)
    {
        vector<string>::const_iterator it = find(headings.begin(), headings.end(), name);
        BOOST_REQUIRE_MESSAGE(it != headings.end(), "Missing column " + name);
        return static_cast<size_t>(it - headings.begin());
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

    void checkExposure(const vector<vector<double>>& rows, size_t mtmColumn, size_t eeColumn, size_t neeColumn)
    {
        for (size_t i = 0; i < rows.size(); ++i)
        {
            double mtm = rows[i][mtmColumn];
            BOOST_CHECK(abs(rows[i][eeColumn] - max(0.0, mtm)) < valueTolerance);
            BOOST_CHECK(abs(rows[i][neeColumn] - min(0.0, mtm)) < valueTolerance);
        }
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
    std::shared_ptr<Portfolio> portfolio(new Portfolio(contracts));
    std::shared_ptr<PortfolioEvolver> evolver = evolverFor(portfolio, re, simulationDates);
    BOOST_REQUIRE_MESSAGE(evolver->isOK(), evolver->getErrorMessagesAsString());

    vector<Date> dates;
    vector<string> headings;
    vector<vector<double>> forwardRows;
    evolver->getSpotPathDetail(forwardSpots, dates, headings, forwardRows);
    BOOST_REQUIRE_EQUAL(forwardRows.size(), simulationDates.size());
    BOOST_REQUIRE_EQUAL(dates.front(), anchor);

    size_t mtmColumn = column(headings, "MtM");
    size_t eeColumn = column(headings, "EE");
    size_t neeColumn = column(headings, "NEE");
    double anchorValue = portfolio->getValue(re);
    BOOST_CHECK(abs(forwardRows[0][mtmColumn] - anchorValue) < valueTolerance);
    checkExposure(forwardRows, mtmColumn, eeColumn, neeColumn);

    std::shared_ptr<RatesEnvironment> shockedMarket = europeanMarket(gr, anchor, manditoryDates);
    std::shared_ptr<EuropeanOption> shockedOption = soldPut(anchor, strike);
    vector<std::shared_ptr<Contract>> shockedContracts;
    shockedContracts.push_back(shockedOption);
    std::shared_ptr<Portfolio> shockedPortfolio(new Portfolio(shockedContracts));
    std::shared_ptr<PortfolioEvolver> shockedEvolver = evolverFor(shockedPortfolio, shockedMarket, simulationDates);
    BOOST_REQUIRE_MESSAGE(shockedEvolver->isOK(), shockedEvolver->getErrorMessagesAsString());
    vector<Date> shockedDates;
    vector<string> shockedHeadings;
    vector<vector<double>> shockedRows;
    shockedEvolver->getSpotPathDetail(shockedSpots, shockedDates, shockedHeadings, shockedRows);
    size_t shockedMtmColumn = column(shockedHeadings, "MtM");
    BOOST_CHECK(shockedRows[1][shockedMtmColumn] < forwardRows[1][mtmColumn]);

    bool useMyThreshold = true;
    double myThreshold = 1000.0;
    bool useCptyThreshold = true;
    double cptyThreshold = 1000.0;
    double minimumTransferAmount = 500.0;
    double rounding = 100.0;
    std::shared_ptr<Margin> margin(new SimpleMargin(anchor,
                                                     useMyThreshold,
                                                     myThreshold,
                                                     useCptyThreshold,
                                                     cptyThreshold,
                                                     minimumTransferAmount,
                                                     rounding));
    std::shared_ptr<RatesEnvironment> marginMarket = europeanMarket(gr, anchor, manditoryDates);
    std::shared_ptr<EuropeanOption> marginOption = soldPut(anchor, strike);
    vector<std::shared_ptr<Contract>> marginContracts;
    marginContracts.push_back(marginOption);
    std::shared_ptr<Portfolio> marginedPortfolio(new Portfolio(marginContracts, margin));
    std::shared_ptr<PortfolioEvolver> marginEvolver = evolverFor(marginedPortfolio, marginMarket, simulationDates);
    BOOST_REQUIRE_MESSAGE(marginEvolver->isOK(), marginEvolver->getErrorMessagesAsString());
    vector<Date> marginDates;
    vector<string> marginHeadings;
    vector<vector<double>> marginRows;
    marginEvolver->getSpotPathDetail(forwardSpots, marginDates, marginHeadings, marginRows);
    size_t marginMtmColumn = column(marginHeadings, "MtM");
    size_t marginColumn = column(marginHeadings, "Margin");
    vector<double> mtmPath(marginRows.size());
    for (size_t i = 0; i < marginRows.size(); ++i)
    {
        mtmPath[i] = marginRows[i][marginMtmColumn];
    }
    std::shared_ptr<SimpleMargin> agreement(new SimpleMargin(anchor,
                                                              useMyThreshold,
                                                              myThreshold,
                                                              useCptyThreshold,
                                                              cptyThreshold,
                                                              minimumTransferAmount,
                                                              rounding));
    vector<double> expectedMargin;
    agreement->getFutureMarginBalancesAlongPath(mtmPath, expectedMargin);
    BOOST_REQUIRE_EQUAL(expectedMargin.size(), mtmPath.size());
    for (size_t i = 0; i < expectedMargin.size(); ++i)
    {
        BOOST_CHECK(abs(marginRows[i][marginColumn] - expectedMargin[i]) < valueTolerance);
    }

    std::shared_ptr<RegCapUnderlyingFactors> factors(new CommodityRegCapFactors);
    std::shared_ptr<RegCapCreditParameters> credit(new RegCapCreditParameters(1.5, 0.1));
    std::shared_ptr<Margin> capitalMargin(new SimpleMargin(anchor,
                                                            useMyThreshold,
                                                            myThreshold,
                                                            useCptyThreshold,
                                                            cptyThreshold,
                                                            minimumTransferAmount,
                                                            rounding));
    std::shared_ptr<RatesEnvironment> capitalMarket = europeanMarket(gr, anchor, manditoryDates);
    std::shared_ptr<EuropeanOption> capitalOption = soldPut(anchor, strike);
    vector<std::shared_ptr<Contract>> capitalContracts;
    capitalContracts.push_back(capitalOption);
    std::shared_ptr<PortfolioB2CEM> capitalPortfolio(new PortfolioB2CEM(capitalContracts,
                                                                         factors,
                                                                         credit,
                                                                         true,
                                                                         capitalMargin));
    std::shared_ptr<SingleFactorEconomicScenarioGenerator> capitalEvolverGenerator(
        new SingleFactorEconomicScenarioGenerator(capitalMarket));
    std::shared_ptr<PortfolioEvolver> capitalEvolver(
        new PortfolioEvolver(capitalPortfolio,
                             capitalEvolverGenerator,
                             simulationDates,
                             capitalPortfolio->regulatoryCapital()));
    BOOST_REQUIRE_MESSAGE(capitalEvolver->isOK(), capitalEvolver->getErrorMessagesAsString());
    vector<Date> capitalDates;
    vector<string> capitalHeadings;
    vector<vector<double>> capitalRows;
    capitalEvolver->getSpotPathDetail(forwardSpots, capitalDates, capitalHeadings, capitalRows);
    size_t regCapColumn = column(capitalHeadings, "Reg Cap");
    size_t capitalMarginColumn = column(capitalHeadings, "Margin");
    std::shared_ptr<SingleFactorEconomicScenarioGenerator> capitalGenerator(
        new SingleFactorEconomicScenarioGenerator(capitalMarket));
    BOOST_REQUIRE(capitalGenerator->setSimulationDates(simulationDates));
    std::shared_ptr<Path> capitalPath = capitalGenerator->getPathForSpot(forwardSpots);
    for (size_t i = 0; i < capitalRows.size(); ++i)
    {
        capitalPortfolio->setMarginBalance(capitalRows[i][capitalMarginColumn]);
        double expectedCapital = capitalPortfolio->getRegulatoryCapital(capitalPath->getRatesEnvironment(i));
        BOOST_CHECK(abs(capitalRows[i][regCapColumn] - expectedCapital) < valueTolerance);
    }

    std::shared_ptr<RatesEnvironment> monteCarloMarket = europeanMarket(gr, anchor, manditoryDates);
    std::shared_ptr<EuropeanOption> monteCarloOption = soldPut(anchor, strike);
    vector<std::shared_ptr<Contract>> monteCarloContracts;
    monteCarloContracts.push_back(monteCarloOption);
    std::shared_ptr<Portfolio> monteCarloPortfolio(new Portfolio(monteCarloContracts));
    std::shared_ptr<SingleFactorEconomicScenarioGenerator> monteCarloGenerator(
        new SingleFactorEconomicScenarioGenerator(monteCarloMarket));
    std::shared_ptr<PortfolioEvolver> monteCarloEvolver(
        new PortfolioEvolver(monteCarloPortfolio, monteCarloGenerator, simulationDates));
    BOOST_REQUIRE_MESSAGE(monteCarloEvolver->isOK(), monteCarloEvolver->getErrorMessagesAsString());

    const long seed = 17;
    const unsigned long paths = 2;
    monteCarloGenerator->setRandomSeed(seed);
    vector<vector<double>> sampledMtM;
    sampledMtM.push_back(markToMarketAlong(monteCarloPortfolio, monteCarloGenerator->getRandomPath()));
    sampledMtM.push_back(markToMarketAlong(monteCarloPortfolio, monteCarloGenerator->getAntitheticPath()));

    monteCarloGenerator->setRandomSeed(seed);
    monteCarloEvolver->runMonteCarlo(paths);
    vector<Date> monteCarloDates;
    vector<string> monteCarloHeadings;
    vector<vector<double>> monteCarloRows;
    monteCarloEvolver->getMonteCarloDetail(monteCarloDates, monteCarloHeadings, monteCarloRows);
    BOOST_REQUIRE_EQUAL(monteCarloRows.size(), simulationDates.size());
    size_t averageMtmColumn = column(monteCarloHeadings, "Ave MtM");
    size_t averageEeColumn = column(monteCarloHeadings, "EE");
    size_t averageNeeColumn = column(monteCarloHeadings, "NEE");
    for (size_t i = 0; i < monteCarloRows.size(); ++i)
    {
        double averageMtm = 0.5 * (sampledMtM[0][i] + sampledMtM[1][i]);
        double averageEe = 0.5 * (max(0.0, sampledMtM[0][i]) + max(0.0, sampledMtM[1][i]));
        double averageNee = 0.5 * (min(0.0, sampledMtM[0][i]) + min(0.0, sampledMtM[1][i]));
        BOOST_CHECK(abs(monteCarloRows[i][averageMtmColumn] - averageMtm) < valueTolerance);
        BOOST_CHECK(abs(monteCarloRows[i][averageEeColumn] - averageEe) < valueTolerance);
        BOOST_CHECK(abs(monteCarloRows[i][averageNeeColumn] - averageNee) < valueTolerance);
    }

    monteCarloGenerator->setRandomSeed(seed);
    monteCarloEvolver->runMonteCarlo(1);
    vector<Date> oddDates;
    vector<string> oddHeadings;
    vector<vector<double>> oddRows;
    monteCarloEvolver->getMonteCarloDetail(oddDates, oddHeadings, oddRows);
    BOOST_REQUIRE_EQUAL(oddRows.size(), monteCarloRows.size());
    for (size_t i = 0; i < oddRows.size(); ++i)
    {
        BOOST_CHECK(abs(oddRows[i][averageMtmColumn] - monteCarloRows[i][averageMtmColumn]) < valueTolerance);
        BOOST_CHECK(abs(oddRows[i][averageEeColumn] - monteCarloRows[i][averageEeColumn]) < valueTolerance);
        BOOST_CHECK(abs(oddRows[i][averageNeeColumn] - monteCarloRows[i][averageNeeColumn]) < valueTolerance);
    }

    monteCarloEvolver->runMonteCarlo(0);
    BOOST_CHECK(monteCarloEvolver->getErrorMessagesAsString().find("Number of paths") != string::npos);

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
