#include "OptionContractTest.h"

using namespace std;
using namespace QuantLib;
using namespace boost;


using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;


std::shared_ptr<EuropeanOption> OptionContactTest::getDefaultEuropeanOption(Date anchorDate)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date optionMaturity = nullCalendar->advance(anchorDate, 2, Years);
    Date optionSettlement = nullCalendar->advance(optionMaturity, 2, Days);
    double strike = 100;
    PutCall type = PUT;
    double defPremium = 3.4;
    double volume = 10000;

    return std::shared_ptr<EuropeanOption>(new  EuropeanOption(optionMaturity, optionSettlement, strike, type, volume, optionSettlement, defPremium, BUY));
}


void OptionContactTest::testBuySell()
{
    BOOST_TEST_MESSAGE("Testing OptionContract ...");
    Date anchorDate(1,Jan,2013);
    std::shared_ptr<EuropeanOption> contract = getDefaultEuropeanOption(anchorDate);
    BuySell bs = SELL;
    contract->setBuySell(bs);
    BuySell bs2 = contract->getBuySell();
    BOOST_CHECK(bs == bs2);
}

void OptionContactTest::testStrike()
{
    Date anchorDate(1,Jan,2013);
    GenerateRates gr = GenerateRates();

    std::shared_ptr<EuropeanOption> contract = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForEuropeanOption(anchorDate, vector<Date>(0));
    BOOST_CHECK(contract->isOK());
    double strike = -1;
    contract->setStrike(strike);
    contract->validate();
    BOOST_CHECK(!contract->isOK());
    BOOST_CHECK(abs(contract->getStrike()-strike)<EPSILON_FOR_DOUBLES);
}

void OptionContactTest::testVolume()
{
    Date anchorDate(1,Jan,2013);
    GenerateRates gr = GenerateRates();

    std::shared_ptr<EuropeanOption> contract = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForEuropeanOption(anchorDate, vector<Date>(0));
    BOOST_CHECK(contract->isOK());
    double volume = -1;
    contract->setVolume(volume);
    BOOST_CHECK(contract->isOK());
    BOOST_CHECK(abs(contract->getVolume()+volume)<EPSILON_FOR_DOUBLES);
}

void OptionContactTest::testMaturityDate()
{
    Date anchorDate(1,Jan,2013);
    GenerateRates gr = GenerateRates();

    std::shared_ptr<EuropeanOption> contract = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForEuropeanOption(anchorDate, vector<Date>(0));
    BOOST_CHECK(contract->isOK());    
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date optionMaturity = nullCalendar->advance(anchorDate, 3, Years);
    contract->setMaturityDate(optionMaturity);
    contract->validate();
    BOOST_CHECK(!contract->isOK());
    BOOST_CHECK(contract->getMaturityDate() == optionMaturity);
}

void OptionContactTest::testPremiumAmount()
{
    Date anchorDate(1,Jan,2013);
    GenerateRates gr = GenerateRates();

    std::shared_ptr<EuropeanOption> contract = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForEuropeanOption(anchorDate, vector<Date>(0));
    BOOST_CHECK(contract->isOK());    

    double ppAmount = -10;
    contract->setPremiumAmount(ppAmount);
    BOOST_CHECK(abs(contract->getPremiumAmount() + ppAmount)<EPSILON_FOR_DOUBLES);    

}

void OptionContactTest::testType()
{
    Date anchorDate(1,Jan,2013);
    GenerateRates gr = GenerateRates();

    std::shared_ptr<EuropeanOption> contract = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForEuropeanOption(anchorDate, vector<Date>(0));
    BOOST_CHECK(contract->getType() == PUT);  
    double valuePut = contract->getValue(re);
    contract->setType(CALL);
    BOOST_CHECK(contract->getType() == CALL);    
    double valueCall = contract->getValue(re);
    std::shared_ptr<EuropeanOption> freshCall = getDefaultEuropeanOption(anchorDate);
    freshCall->setType(CALL);
    BOOST_CHECK(abs(valueCall - freshCall->getValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(valuePut - valueCall)>EPSILON_FOR_DOUBLES); 
}

void OptionContactTest::testPutCallParity()
{
    Date anchorDate(1,Jan,2013);
    GenerateRates gr = GenerateRates();

    std::shared_ptr<EuropeanOption> put = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<EuropeanOption> call = getDefaultEuropeanOption(anchorDate);
    call->setType(CALL);
    call->setBuySell(SELL);
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForEuropeanOption(anchorDate, vector<Date>(0));

    EuropeanForward forward(put->getMaturityDate(), put->getSettlementDate(), put->getStrike(), put->getVolume(), SELL);
    BOOST_CHECK(abs(put->getValue(re) + call->getValue(re) - forward.getValue(re))<1e-10);
}

void OptionContactTest::testValue()
{
    Date anchorDate(1,Jan,2013);
    GenerateRates gr = GenerateRates();

    std::shared_ptr<DayCounter> dayCounter = gr.defaultDayCounter;;
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;

    Date tPlus2 = nullCalendar->advance(anchorDate, 2, Days);
    Date optionMaturity = nullCalendar->advance(anchorDate, 2, Years);
    Date optionSettlement = nullCalendar->advance(optionMaturity, 2, Days);

    vector<Date> manditoryDates;
    manditoryDates.push_back(tPlus2);
    manditoryDates.push_back(optionMaturity);
    manditoryDates.push_back(optionSettlement);

    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForEuropeanOption(anchorDate, manditoryDates);

    double strike = 100;
    PutCall type = PUT;
    double defPremium = 3.4;

    EuropeanOption contract(optionMaturity, optionSettlement, strike, type, 1.0, anchorDate, 0.0, BUY);
    EuropeanOption contract2(optionMaturity, optionSettlement, strike, type, 2.0, anchorDate, 0.0, BUY);
    EuropeanOption deferredPremiumContract(optionMaturity, optionSettlement, strike, type, 1, optionSettlement, defPremium, BUY);
    EuropeanOption deferredPremiumContract2(optionMaturity, optionSettlement, strike, type, 2, optionSettlement, defPremium, BUY);
    
    double yte = dayCounter->yearFraction(anchorDate, optionMaturity);
    double yts = dayCounter->yearFraction(anchorDate, optionSettlement);
    double fwd = re->getFRS()->getForward(optionSettlement);
    Black76Put simplePut(fwd, strike, gr.defaultVolatiltiy * sqrt(yte), exp(-gr.defaultNACCRate * yts));
    double pvDefPremium = defPremium * re->getDRS()->getDiscountFactor(optionSettlement);

    // 1) Before maturity
    contract.validate();
    BOOST_CHECK(abs(simplePut.getPremium() - contract.getOptionValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(simplePut.getPremium() - contract.getValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(contract2.getOptionValue(re) - 2 * contract.getOptionValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(contract2.getValue(re) - 2 * contract.getValue(re)) < EPSILON_FOR_DOUBLES);

    BOOST_CHECK(abs(deferredPremiumContract2.getOptionValue(re) - 2 * deferredPremiumContract.getOptionValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(deferredPremiumContract2.getOptionValue(re) - 2 * (pvDefPremium + deferredPremiumContract.getValue(re))) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(simplePut.getPremium() - deferredPremiumContract.getOptionValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(simplePut.getPremium() - pvDefPremium - deferredPremiumContract.getValue(re)) < EPSILON_FOR_DOUBLES);
    
    
    // 3) After settlement
    Date dayAfterSettlement = nullCalendar->advance(optionSettlement, 1, Days);
    manditoryDates.clear();
    re = gr.getRatesEnvironmentForEuropeanOption(dayAfterSettlement, manditoryDates);
    BOOST_CHECK(abs(contract2.getOptionValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(deferredPremiumContract2.getOptionValue(re)) < EPSILON_FOR_DOUBLES);    
}

void OptionContactTest::testGetPayoffAfterMaturity()
{
    std::shared_ptr<DayCounter> dayCounter = std::shared_ptr<DayCounter>(new Actual365Fixed());
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());

    Date anchorDate(1,Jan,2013);

    std::shared_ptr<EuropeanOption> option = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<EuropeanOption> option2 = getDefaultEuropeanOption(anchorDate);
    option2->setType(CALL);

    double v1 = option->getVolume();
    double p1 = option->getPremiumAmount();
    double k1 = option->getStrike();
    double sign1 = (option->getBuySell() == BUY ? 1 : -1);

    double maturitySpot = k1-10;
    BOOST_CHECK(abs(option->getPayoff(maturitySpot) - (k1 - maturitySpot - p1) * v1 * sign1)< 1e-14);
    maturitySpot = k1+10;
    BOOST_CHECK(abs(option->getPayoff(maturitySpot) - (- p1) * v1 * sign1)< 1e-14);
    BOOST_CHECK(abs(option2->getPayoff(maturitySpot) - (maturitySpot - k1 - p1) * v1 * sign1)< 1e-14);
}


void OptionContactTest::testEuropeanOptionWithMoneynessVol() 
{
    BOOST_TEST_MESSAGE("Testing European Option with moneyness vol ...");

    std::shared_ptr<RatesEnvironment> re;
    GenerateRates gr = GenerateRates();

    std::shared_ptr<DayCounter> dayCounter = gr.defaultDayCounter;
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;

    Date anchorDate = gr.defaultAnchorDate;
    Date tPlus2 = nullCalendar->advance(anchorDate, 2, Days);
    Date optionMaturity = nullCalendar->advance(anchorDate, 1, Years);
    Date optionSettlement = nullCalendar->advance(optionMaturity, 2, Days);

    vector<Date> manditoryDates;
    manditoryDates.push_back(tPlus2);
    manditoryDates.push_back(optionMaturity);
    manditoryDates.push_back(optionSettlement);

    double spot = gr.defaultSpot;
    double naccRFR = gr.defaultNACCRate;
    double naccIRDiff = gr.defaultNACCIRDiff;    

    std::shared_ptr<DiscountCurve> dc = gr.getDefaultDiscountCurve(anchorDate);
    std::shared_ptr<ForwardCurveDeterministic> fc = gr.getSimpleForwardCurve(dayCounter, nullCalendar, anchorDate, 10, spot, naccIRDiff, manditoryDates);
    std::shared_ptr<VolatilitySurfaceMoneyness> vs = gr.getDefaultMoneynessSurface(anchorDate);
    re = std::shared_ptr<RatesEnvironment>(new RatesEnvironment(anchorDate));
    re->setDRS(dc);
    re->setFRS(fc);
    re->setVRS(vs);

    double strike = 100;
    PutCall type = PUT;
    double defPremium = 3.4;
    double volume = 2.5;

    EuropeanOption contract(optionMaturity, optionSettlement, strike, type, volume, anchorDate, 0.0, BUY);
    EuropeanOption deferredPremiumContract(optionMaturity, optionSettlement, strike, type, volume, optionSettlement, defPremium, BUY);

    double yte = dayCounter->yearFraction(anchorDate, optionMaturity);
    double yts = dayCounter->yearFraction(anchorDate, optionSettlement);
    double fwd = fc->getForward(optionSettlement);
    double df = dc->getDiscountFactor(optionSettlement);
    double sd = vs->getStandardDeviation(optionMaturity, strike, fwd);

    Black76Put simplePut(fwd, strike, sd, df);
    double pvDefPremium = defPremium * df;
    
    // 1) Before maturity
    BOOST_CHECK(abs(simplePut.getPremium() * volume - contract.getOptionValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(simplePut.getPremium() * volume - contract.getValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(simplePut.getPremium() * volume - deferredPremiumContract.getOptionValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs((simplePut.getPremium() - pvDefPremium) * volume - deferredPremiumContract.getValue(re)) < EPSILON_FOR_DOUBLES);
    
    // 3) After settlement
    Date dayAfterSettlement = nullCalendar->advance(optionSettlement, 1, Days);
    re = std::shared_ptr<RatesEnvironment>(new RatesEnvironment(dayAfterSettlement));
    manditoryDates.clear();
    dc = gr.getDiscountCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, naccRFR);
    fc = gr.getSimpleForwardCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, spot, naccIRDiff);
    vs = gr.getDefaultMoneynessSurface(dayAfterSettlement);
    re->setDRS(dc);
    re->setFRS(fc);
    re->setVRS(vs);

    BOOST_CHECK(abs(contract.getOptionValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(deferredPremiumContract.getOptionValue(re)) < EPSILON_FOR_DOUBLES);        
}


test_suite* OptionContactTest::suite() 
{
    test_suite* suite = BOOST_TEST_SUITE("Option Contract");

    suite->add(BOOST_TEST_CASE(&OptionContactTest::testBuySell));
    suite->add(BOOST_TEST_CASE(&OptionContactTest::testStrike));
    suite->add(BOOST_TEST_CASE(&OptionContactTest::testVolume));
    suite->add(BOOST_TEST_CASE(&OptionContactTest::testMaturityDate));
    suite->add(BOOST_TEST_CASE(&OptionContactTest::testPremiumAmount));
    suite->add(BOOST_TEST_CASE(&OptionContactTest::testType));

    suite->add(BOOST_TEST_CASE(&OptionContactTest::testValue));
    suite->add(BOOST_TEST_CASE(&OptionContactTest::testGetPayoffAfterMaturity));       
    suite->add(BOOST_TEST_CASE(&OptionContactTest::testPutCallParity));

    suite->add(BOOST_TEST_CASE(&OptionContactTest::testEuropeanOptionWithMoneynessVol));
    
    return suite;
}
