#include "PortfolioTest.h"

#include <sjd/contracts/Portfolio/Structures/Structure.h>
#include <sjd/tools/Errors.h>

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;

std::shared_ptr<RegCapUnderlyingFactors> PortfolioTest::getDefaultRegCapUnderlyingFactors()
{
    return std::shared_ptr<RegCapUnderlyingFactors>(std::make_shared<CommodityRegCapFactors>());
}

std::shared_ptr<RegCapCreditParameters> PortfolioTest::getDefaultRegCreditParameters()
{
    double counterpartyWeighting = 1.5;
    double capitalCharge = .1;
    return std::make_shared<RegCapCreditParameters>(counterpartyWeighting, capitalCharge);
}

void PortfolioTest::testStructureCreation()
{
    BOOST_TEST_MESSAGE("Testing Portfolios ...");

    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    double structureVolume = 10000;
    double premiumRecievable = 2000;
    std::shared_ptr<EuropeanForward> fwdSale1 = GenerateContracts::getDefaultEuropeanForward(
        anchorDate, yearsToSettlement, 112, structureVolume, BuySell::SELL);
    std::shared_ptr<EuropeanForward> fwdSale2 = GenerateContracts::getDefaultEuropeanForward(
        anchorDate, yearsToSettlement, 112, structureVolume, BuySell::SELL);

    std::shared_ptr<SingleInstrumentContract> cf1 = GenerateContracts::getDefaultFixedCashflowWithPremium(
        anchorDate, yearsToSettlement, premiumRecievable, BuySell::BUY);
    std::shared_ptr<SingleInstrumentContract> cf2 = GenerateContracts::getDefaultFixedCashflowWithPremium(
        anchorDate, yearsToSettlement, premiumRecievable, BuySell::BUY);

    std::shared_ptr<EuropeanOption> option1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 115, structureVolume, BuySell::BUY, PutCall::CALL, 0);
    std::shared_ptr<EuropeanOption> option2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 115, structureVolume, BuySell::BUY, PutCall::CALL, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> siContracts;
    siContracts.push_back(fwdSale1);
    siContracts.push_back(cf1);
    siContracts.push_back(option1);
    std::shared_ptr<Portfolio> p1 = std::make_shared<Portfolio>(siContracts);
    p1->validate(re);
    BOOST_REQUIRE(p1->isOK());
    BOOST_REQUIRE(!p1->isMargined());

    vector<std::shared_ptr<Contract>> contracts;
    contracts.push_back(fwdSale2);
    contracts.push_back(cf2);
    contracts.push_back(option2);
    std::shared_ptr<Portfolio> p2 = std::make_shared<Portfolio>(contracts);
    p2->validate(re);
    BOOST_REQUIRE(p2->isOK());
    BOOST_REQUIRE(!p2->isMargined());

    BOOST_CHECK(abs(p1->getValue(re) - p2->getValue(re)) < EPSILON_FOR_DOUBLES);

    bool useMyThreshold = true;
    bool useCptyThreshold = true;
    double myThreshold = 20000.0, cptyThreshold = 10000.0, minimumTransferAmount = 5000.0, rounding = 100.0;
    std::shared_ptr<Margin> margin1 =
        std::make_shared<SimpleMargin>(anchorDate, useMyThreshold, myThreshold, useCptyThreshold,
                                       cptyThreshold, minimumTransferAmount, rounding);
    p1 = std::make_shared<Portfolio>(siContracts, margin1);
    p1->validate(re);
    BOOST_REQUIRE(p1->isOK());
    BOOST_REQUIRE(p1->isMargined());
    BOOST_CHECK(abs(p1->getValue(re) - p2->getValue(re)) < EPSILON_FOR_DOUBLES);

    std::shared_ptr<Margin> margin2 =
        std::make_shared<SimpleMargin>(anchorDate, useMyThreshold, myThreshold, useCptyThreshold,
                                       cptyThreshold, minimumTransferAmount, rounding);
    p2 = std::make_shared<Portfolio>(contracts, margin2);
    p2->validate(re);
    BOOST_REQUIRE(p2->isOK());
    BOOST_REQUIRE(p2->isMargined());
    BOOST_CHECK(abs(p1->getValue(re) - p2->getValue(re)) < EPSILON_FOR_DOUBLES);

    BOOST_CHECK(abs(p1->getMarginCall(re) - p2->getMarginCall(re)) < EPSILON_FOR_DOUBLES);

    double newBalance = -50000.0;
    p1->setMarginBalance(newBalance);
    BOOST_CHECK(abs(p1->getMarginCall(re) - p2->getMarginCall(re)) > EPSILON_FOR_DOUBLES);
    p2->getMargin()->setMarginBalance(newBalance);
    BOOST_CHECK(abs(p1->getMarginCall(re) - p2->getMarginCall(re)) < EPSILON_FOR_DOUBLES);

    p1->updateMarginBalanceWithCall(p1->getMarginCall(re));
    BOOST_CHECK(abs(p1->getMarginCall(re) - p2->getMarginCall(re)) > EPSILON_FOR_DOUBLES);
    p2->setMarginBalance(p2->getMarginBalance() + p2->getMarginCall(re));
    BOOST_CHECK(abs(p1->getMarginCall(re) - p2->getMarginCall(re)) < EPSILON_FOR_DOUBLES);
}

void PortfolioTest::testPortfolioB2CEM()
{
    BOOST_TEST_MESSAGE("Testing B2 CEM ...");

    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    [[maybe_unused]] double structureVolume = 10000;
    [[maybe_unused]] double premiumRecievable = 2000;
    int numberOfAveragingDates = 20;
    double strike = 100;
    double volume = 5000;
    BuySell bs = BuySell::SELL;
    PutCall pc = PutCall::PUT;
    double premium = 5.0;
    bool dp = true;

    std::shared_ptr<AsianOption> put = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingDates, strike, volume, bs, pc, premium, dp);

    vector<std::shared_ptr<Contract>> contracts;
    contracts.push_back(put);

    std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors =
        PortfolioTest::getDefaultRegCapUnderlyingFactors();
    std::shared_ptr<RegCapCreditParameters> regCapCreditParameters =
        PortfolioTest::getDefaultRegCreditParameters();
    bool optimise = true;

    std::shared_ptr<PortfolioB2CEM> unmarginedPortfolio =
        std::make_shared<PortfolioB2CEM>(contracts, regCapAssetFactors, regCapCreditParameters, optimise);
    BOOST_REQUIRE(unmarginedPortfolio->isOK());

    bool useMyThreshold = true, useCptyThreshold = true;
    double myThreshold = 2000000.0, cptyThreshold = 1000000.0, minimumTransferAmount = 500000.0,
           rounding = 10000.0;
    std::shared_ptr<SimpleMargin> margin =
        std::make_shared<SimpleMargin>(anchorDate, useMyThreshold, myThreshold, useCptyThreshold,
                                       cptyThreshold, minimumTransferAmount, rounding);
    margin->setMarginBalance(0);
    margin->updateBalanceForGivenMtM(unmarginedPortfolio->getValue(re));

    std::shared_ptr<Calendar> calendar = std::make_shared<WeekendsOnly>();
    [[maybe_unused]] size_t holdingPeriodInBusinessDays = 10;

    std::shared_ptr<PortfolioB2CEM> marginedPortfolio = std::make_shared<PortfolioB2CEM>(
        contracts, regCapAssetFactors, regCapCreditParameters, optimise, margin);

    // marginedPortfolio->setMarginParameters(margin, calendar, holdingPeriodInBusinessDays);
    BOOST_REQUIRE(marginedPortfolio->isOK());

    /*
    set<Date> exposureDates;
    for (size_t i = 1; i < 13; ++i)
    {
        exposureDates.insert(calendar->advance(anchorDate, static_cast<int>(i), Months));
    }
    set<Date>::iterator exposureDateIt;
    set<Date> simulationDates;

    for (exposureDateIt = exposureDates.begin(); exposureDateIt != exposureDates.end(); ++exposureDateIt)
    {
        Date simDate = calendar->advance(*exposureDateIt, - ((int) holdingPeriodInBusinessDays), Days);
        if (simDate < anchorDate)
        {
            simDate = anchorDate;
        }
        simulationDates.insert(simDate);
        simulationDates.insert(*exposureDateIt);
    }

    marginedPortfolio->setForwardValuationDates(exposureDates);
    set<Date> simDatesCheck = marginedPortfolio->getSimulationDates();
    BOOST_REQUIRE(simulationDates.size() == simDatesCheck.size());
    set<Date>::iterator loopIt, searchIt;
    for (loopIt = simulationDates.begin(); loopIt != simulationDates.end(); ++loopIt)
    {
        searchIt = find(simDatesCheck.begin(), simDatesCheck.end(), *loopIt);
        BOOST_REQUIRE(searchIt != simDatesCheck.end());
    }

    unmarginedPortfolio->setForwardValuationDates(simulationDates);

    BOOST_REQUIRE(unmarginedPortfolio->isOK());
    BOOST_REQUIRE(marginedPortfolio->isOK());

    std::shared_ptr<SingleFactorGBMPathGenerator> mcPathGenerator =
    std::make_shared<SingleFactorGBMPathGenerator>(fc, vc);

    marginedPortfolio->setPathGenerator(mcPathGenerator);
    unmarginedPortfolio->setPathGenerator(mcPathGenerator);

    mcPathGenerator->setSimulationDates(simulationDates);
    vector<double> unmarginedSpotCurve = mcPathGenerator->getConfidenceInterval(0.95);

    vector<double> marginedValues = marginedPortfolio->evolveAlongPath(unmarginedSpotCurve);
    vector<double> unmarginedValues = unmarginedPortfolio->evolveAlongPath(unmarginedSpotCurve);
    BOOST_REQUIRE(marginedValues.size() == unmarginedValues.size());
    for (size_t i = 0; i < unmarginedValues.size(); ++i)
    {
        BOOST_CHECK(abs(marginedValues[i] - unmarginedValues[i]) < EPSILON_FOR_DOUBLES);
    }
    */
}

void PortfolioTest::testInvalidRegulatoryCapitalIsNaN()
{
    BOOST_TEST_MESSAGE("Testing B2 CEM on a netting set that cannot be offset ...");

    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    // Different settlement dates disqualify the pair from regulatory capital offset.
    vector<std::shared_ptr<SingleInstrumentContract>> legs;
    legs.push_back(GenerateContracts::getDefaultEuropeanForward(anchorDate, 2, 112, 10000, BuySell::BUY));
    legs.push_back(GenerateContracts::getDefaultEuropeanOption(anchorDate, 3, 115, 10000, BuySell::SELL,
                                                               PutCall::CALL, 0));
    vector<std::shared_ptr<Contract>> nettingSets;
    nettingSets.push_back(std::make_shared<Portfolio>(legs));

    bool optimise = false;
    CurrentExposureMethod capital(nettingSets, legs, std::shared_ptr<Margin>(),
                                  getDefaultRegCapUnderlyingFactors(), getDefaultRegCreditParameters(),
                                  optimise);
    BOOST_REQUIRE(capital.isOK());
    capital.validate();
    BOOST_REQUIRE(capital.hasErrors());
    const size_t issues = capital.validationReport().getIssues().size();
    capital.validate();
    BOOST_CHECK_EQUAL(capital.validationReport().getIssues().size(), issues);
    capital.validate(re);
    const size_t issuesWithRates = capital.validationReport().getIssues().size();
    BOOST_REQUIRE(issuesWithRates > 0);
    capital.validate(re);
    BOOST_CHECK_EQUAL(capital.validationReport().getIssues().size(), issuesWithRates);
#ifndef NDEBUG
    BOOST_CHECK_THROW(capital.getRegulatoryCapital(re), sjd::Error);
#endif

    PortfolioB2CEM book(nettingSets, getDefaultRegCapUnderlyingFactors(), getDefaultRegCreditParameters(),
                        optimise);
    book.validate();
    BOOST_CHECK(!book.isOK());
    book.validate(re);
    BOOST_CHECK(!book.isOK());
}

void PortfolioTest::testNullMarginAndUnknownFactors()
{
    BOOST_TEST_MESSAGE("Testing a null margin and an unknown regulatory factor name");

    vector<std::shared_ptr<SingleInstrumentContract>> contracts;
    std::shared_ptr<Portfolio> portfolio(std::make_shared<Portfolio>(contracts, std::shared_ptr<Margin>()));
    BOOST_CHECK(!portfolio->isMargined());
    portfolio->validate();
    std::string emptyMessage = portfolio->getErrorMessagesAsString();
    BOOST_CHECK(emptyMessage.find("contains no contracts") != string::npos);
    BOOST_CHECK(emptyMessage.find(", contains") == string::npos);

    BOOST_CHECK_THROW(generateRegCapFactors("not-an-asset-class"), ConfigurationError);

    vector<std::shared_ptr<SingleInstrumentContract>> none;
    Structure empty(none, getDefaultRegCapUnderlyingFactors());
    BOOST_CHECK(!empty.isOK());
    BOOST_CHECK(empty.getSettlementDate() != Date(1, Jan, 1910));
    empty.validate();
    BOOST_CHECK(empty.getErrorMessagesAsString().find("no contracts") != string::npos);
}

test_suite* PortfolioTest::suite()
{

    test_suite* suite = BOOST_TEST_SUITE("Portfolios");
    suite->add(BOOST_TEST_CASE(&PortfolioTest::testStructureCreation));
    suite->add(BOOST_TEST_CASE(&PortfolioTest::testPortfolioB2CEM));
    suite->add(BOOST_TEST_CASE(&PortfolioTest::testInvalidRegulatoryCapitalIsNaN));
    suite->add(BOOST_TEST_CASE(&PortfolioTest::testNullMarginAndUnknownFactors));

    return suite;
}
