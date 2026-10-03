#include "PortfolioTest.h"

using namespace std;
using namespace QuantLib;
using namespace boost;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;



std::shared_ptr<RegCapUnderlyingFactors> PortfolioTest::getDefaultRegCapUnderlyingFactors()
{
    return std::shared_ptr<RegCapUnderlyingFactors>(new CommodityRegCapFactors);
}

std::shared_ptr<RegCapCreditParameters> PortfolioTest::getDefaultRegCreditParameters()
{
    double counterpartyWeighting = 1.5;
    double capitalCharge = .1;
    return std::shared_ptr<RegCapCreditParameters>(new RegCapCreditParameters(counterpartyWeighting, capitalCharge));
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
    std::shared_ptr<EuropeanForward> fwdSale1 = 
        GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToSettlement, 112, structureVolume, SELL);
    std::shared_ptr<EuropeanForward> fwdSale2 = 
        GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToSettlement, 112, structureVolume, SELL);

    std::shared_ptr<SingleInstrumentContract> cf1 = 
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, premiumRecievable, BUY);
    std::shared_ptr<SingleInstrumentContract> cf2 = 
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, premiumRecievable, BUY);

    std::shared_ptr<EuropeanOption> option1 = 
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 115, structureVolume, BUY, CALL, 0);
    std::shared_ptr<EuropeanOption> option2 = 
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 115, structureVolume, BUY, CALL, 0);
    
    vector<std::shared_ptr<SingleInstrumentContract>> siContracts;
    siContracts.push_back(fwdSale1);
    siContracts.push_back(cf1);
    siContracts.push_back(option1);
    std::shared_ptr<Portfolio> p1 = std::shared_ptr<Portfolio>(new Portfolio(siContracts));
    p1->validate(re);
    BOOST_REQUIRE(p1->isOK());
    BOOST_REQUIRE(!p1->isMargined());

    vector<std::shared_ptr<Contract>> contracts;
    contracts.push_back(fwdSale2);
    contracts.push_back(cf2);
    contracts.push_back(option2);
    std::shared_ptr<Portfolio> p2 = std::shared_ptr<Portfolio>(new Portfolio(contracts));
    p2->validate(re);
    BOOST_REQUIRE(p2->isOK());
    BOOST_REQUIRE(!p2->isMargined());

    BOOST_CHECK(abs(p1->getValue(re) - p2->getValue(re)) < EPSILON_FOR_DOUBLES);

    bool useMyThreshold = true; 
    bool useCptyThreshold = true;
    double myThreshold = 20000.0, 
           cptyThreshold = 10000.0, 
           minimumTransferAmount = 5000.0, 
           rounding = 100.0;
    std::shared_ptr<Margin> margin1 = std::shared_ptr<Margin>(
        new SimpleMargin(anchorDate, useMyThreshold, myThreshold, useCptyThreshold, cptyThreshold, minimumTransferAmount, rounding));
    p1 = std::shared_ptr<Portfolio>(new Portfolio(siContracts, margin1));
    p1->validate(re);
    BOOST_REQUIRE(p1->isOK());
    BOOST_REQUIRE(p1->isMargined());
    BOOST_CHECK(abs(p1->getValue(re) - p2->getValue(re)) < EPSILON_FOR_DOUBLES);

    std::shared_ptr<Margin> margin2 = std::shared_ptr<Margin>(
        new SimpleMargin(anchorDate, useMyThreshold, myThreshold, useCptyThreshold, cptyThreshold, minimumTransferAmount, rounding));
    p2 = std::shared_ptr<Portfolio>(new Portfolio(contracts, margin2));
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
    double structureVolume = 10000;
    double premiumRecievable = 2000;
    int numberOfAveragingDates = 20;
    double strike = 100;
    double volume = 5000;
    BuySell bs= SELL;
    PutCall pc = PUT;
    double premium = 5.0;
    bool dp = true;

    std::shared_ptr<AsianOption> put = GenerateContracts::getDefaultAsianOption(anchorDate,  
                                                                                  yearsToSettlement, 
                                                                                  numberOfAveragingDates, 
                                                                                  strike, 
                                                                                  volume, 
                                                                                  bs, 
                                                                                  pc, 
                                                                                  premium, 
                                                                                  dp);

    vector<std::shared_ptr<Contract>> contracts;
    contracts.push_back(put);

    string errorMessage;

    std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors = PortfolioTest::getDefaultRegCapUnderlyingFactors();
    std::shared_ptr<RegCapCreditParameters> regCapCreditParameters = PortfolioTest::getDefaultRegCreditParameters();
    bool optimise = true;

    std::shared_ptr<PortfolioB2CEM> unmarginedPortfolio = std::shared_ptr<PortfolioB2CEM>(
                                                    new PortfolioB2CEM(contracts, 
                                                                       regCapAssetFactors, 
                                                                       regCapCreditParameters, 
                                                                       optimise));
    BOOST_REQUIRE(unmarginedPortfolio->isOK());


    bool useMyThreshold = true, useCptyThreshold = true;
    double myThreshold = 2000000.0, cptyThreshold = 1000000.0, minimumTransferAmount = 500000.0, rounding = 10000.0;
    std::shared_ptr<SimpleMargin> margin = std::shared_ptr<SimpleMargin>(
                                                new SimpleMargin(anchorDate,
                                                                 useMyThreshold, 
                                                                 myThreshold, 
                                                                 useCptyThreshold, 
                                                                 cptyThreshold, 
                                                                 minimumTransferAmount, 
                                                                 rounding));
    margin->setMarginBalance(0);
    margin->updateBalanceForGivenMtM(unmarginedPortfolio->getValue(re));

    std::shared_ptr<Calendar> calendar = std::shared_ptr<Calendar>(new WeekendsOnly());
    size_t holdingPeriodInBusinessDays = 10;

    std::shared_ptr<PortfolioB2CEM> marginedPortfolio = std::shared_ptr<PortfolioB2CEM>(
                                                    new PortfolioB2CEM(contracts, 
                                                                       regCapAssetFactors, 
                                                                       regCapCreditParameters, 
                                                                       optimise,
                                                                       margin));


    //marginedPortfolio->setMarginParameters(margin, calendar, holdingPeriodInBusinessDays);
    BOOST_REQUIRE(marginedPortfolio->isOK());

    /*
    set<Date> exposureDates;
    for (size_t i = 1; i < 13; ++i)
    {
        exposureDates.insert(calendar->advance(anchorDate, i, Months));
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

    std::shared_ptr<SingleFactorGBMPathGenerator> mcPathGenerator = std::shared_ptr<SingleFactorGBMPathGenerator>(
                                                                            new SingleFactorGBMPathGenerator(fc, vc));

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


test_suite* PortfolioTest::suite() 
{
    
    test_suite* suite = BOOST_TEST_SUITE("Portfolios");
    suite->add(BOOST_TEST_CASE(&PortfolioTest::testStructureCreation));
    suite->add(BOOST_TEST_CASE(&PortfolioTest::testPortfolioB2CEM));

    return suite;
    
}
