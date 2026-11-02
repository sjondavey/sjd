#include "AsianOptionTest.h"

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;

std::shared_ptr<AsianOption> AsianOptionContactTest::getDefaultAsianOption(Date anchorDate)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date optionMaturity = nullCalendar->advance(anchorDate, 2, Years);
    set<Date> averagingDates;
    Date d = Date(1, optionMaturity.month(), optionMaturity.year());
    Date nextMonth = nullCalendar->advance(d, 1, Months);
    while (d < nextMonth)
    {
        averagingDates.insert(d);
        d = nullCalendar->advance(d, 1, Days);
    }
    vector<Date> averagingDatesAsVector = vector<Date>(averagingDates.begin(), averagingDates.end());
    Date settlement = nullCalendar->advance(averagingDatesAsVector.back(), 2, Days);

    double strike = 100;
    PutCall type = PUT;
    double defPremium = 3.4;
    double volume = 10000;

    return std::shared_ptr<AsianOption>(
        new AsianOption(averagingDates, settlement, strike, type, volume, settlement, defPremium, BUY));
}

std::shared_ptr<AsianOption> AsianOptionContactTest::getAsianOptionWithMoneynessVol(Date anchorDate)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date optionMaturity = nullCalendar->advance(anchorDate, 2, Years);
    set<Date> averagingDates;
    Date d = Date(1, optionMaturity.month(), optionMaturity.year());
    Date nextMonth = nullCalendar->advance(d, 1, Months);
    while (d < nextMonth)
    {
        averagingDates.insert(d);
        d = nullCalendar->advance(d, 1, Days);
    }
    vector<Date> averagingDatesAsVector = vector<Date>(averagingDates.begin(), averagingDates.end());
    Date settlement = nullCalendar->advance(averagingDatesAsVector.back(), 2, Days);

    double strike = 100;
    PutCall type = PUT;
    double defPremium = 3.4;
    double volume = 10000;

    return std::shared_ptr<AsianOption>(
        new AsianOption(averagingDates, settlement, strike, type, volume, settlement, defPremium, BUY));
}

void AsianOptionContactTest::testMaturityDate()
{
    BOOST_TEST_MESSAGE("Testing AsianOption ...");
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<AsianOption> contract = getDefaultAsianOption(anchorDate);
    Date d = contract->getMaturityDate();
    Date d2 = d + 1;
    // inconsistent maturity date
    contract->setMaturityDate(d2);
    contract->validate();
    BOOST_CHECK(!contract->isOK());
}

void AsianOptionContactTest::testAveragingDates()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<AsianOption> contract = getDefaultAsianOption(anchorDate);
    BOOST_CHECK(contract->isOK());

    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date rateSetDate = nullCalendar->advance(anchorDate, 3, Years); // after settlement
    set<Date> averagingDates;
    Date d = Date(1, rateSetDate.month(), rateSetDate.year());
    Date nextMonth = nullCalendar->advance(d, 1, Months);
    while (d < nextMonth)
    {
        averagingDates.insert(d);
        d = nullCalendar->advance(d, 1, Days);
    }
    contract->setAveragingDates(averagingDates);
    contract->validate();
    BOOST_CHECK(!contract->isOK()); // averaging past settlement
    d = nullCalendar->advance(d, -1, Days);
    BOOST_CHECK(contract->getMaturityDate() == d);
}

void AsianOptionContactTest::testType()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<AsianOption> contract = getDefaultAsianOption(anchorDate);
    BOOST_CHECK(contract->isOK());

    contract->setType(PUT);
    BOOST_CHECK(contract->getType() == PUT);

    contract->setType(CALL);
    BOOST_CHECK(contract->getType() == CALL);
}

void AsianOptionContactTest::testPutCallParity()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<AsianOption> put = getDefaultAsianOption(anchorDate);
    std::shared_ptr<AsianOption> call = getDefaultAsianOption(anchorDate);
    put->setType(PUT);
    put->setBuySell(BUY);
    call->setType(CALL);
    call->setBuySell(SELL);

    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    vector<Date> averagingDates = put->getAveragingDates();
    set<Date> averagingDatesAsSet = set<Date>(averagingDates.begin(), averagingDates.end());
    std::shared_ptr<AsianForward> forward = std::shared_ptr<AsianForward>(new AsianForward(
        averagingDatesAsSet, put->getSettlementDate(), put->getStrike(), put->getVolume(), SELL));

    BOOST_CHECK(abs(put->getValue(re) + call->getValue(re) - forward->getValue(re)) < 1e-10);
}

void AsianOptionContactTest::testValue()
{
    Date anchorDate(1, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    std::shared_ptr<DayCounter> dayCounter = gr.defaultDayCounter;
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;

    std::shared_ptr<AsianOption> call = getDefaultAsianOption(anchorDate);
    call->setType(CALL);

    // 1) Before maturity
    vector<Date> fixingDates, forwardDates;
    call->getFixingAndForwardDates(anchorDate, fixingDates, forwardDates);
    vector<double> forwardRates, fixedRates, sd;
    double strike = call->getStrike();
    for (size_t i = 0; i < forwardDates.size(); ++i)
    {
        forwardRates.push_back(re->getFRS()->getForward(forwardDates[i]));
        sd.push_back(re->getVRS()->getStandardDeviation(forwardDates[i], strike, forwardRates[i]));
    }
    for (size_t i = 0; i < fixingDates.size(); ++i)
    {
        fixedRates.push_back(re->getFixingRS()->getRateSetOn(fixingDates[i]));
    }
    double df = re->getDRS()->getDiscountFactor(call->getSettlementDate());
    std::shared_ptr<AverageRateCall> callFormula =
        std::shared_ptr<AverageRateCall>(new AverageRateCall(forwardRates, fixedRates, strike, sd, df));
    double optionFormulaValue =
        (callFormula->getPremium() - call->getPremiumAmount() * df) * call->getVolume();

    BOOST_CHECK(abs(call->getValue(re) - optionFormulaValue) < 1e-10);

    // 2) In Averaging Month
    Date newAnchorDate = nullCalendar->advance(anchorDate, 2, Years);
    newAnchorDate = nullCalendar->advance(anchorDate, 10, Days);
    re = gr.getRatesEnvironmentForAsianOption(newAnchorDate, vector<Date>(0));

    call->getFixingAndForwardDates(anchorDate, fixingDates, forwardDates);
    forwardRates.clear();
    fixedRates.clear();
    sd.clear();
    for (size_t i = 0; i < forwardDates.size(); ++i)
    {
        forwardRates.push_back(re->getFRS()->getForward(forwardDates[i]));
        sd.push_back(re->getVRS()->getStandardDeviation(forwardDates[i], strike, forwardRates[i]));
    }
    for (size_t i = 0; i < fixingDates.size(); ++i)
    {
        fixedRates.push_back(re->getFixingRS()->getRateSetOn(fixingDates[i]));
    }
    df = re->getDRS()->getDiscountFactor(call->getSettlementDate());
    callFormula =
        std::shared_ptr<AverageRateCall>(new AverageRateCall(forwardRates, fixedRates, strike, sd, df));
    optionFormulaValue = (callFormula->getPremium() - call->getPremiumAmount() * df) * call->getVolume();
    BOOST_CHECK(abs(call->getValue(re) - optionFormulaValue) < 1e-10);

    // 4) After settlement
    Date dayAfterSettlement = nullCalendar->advance(call->getSettlementDate(), 1, Days);
    re = gr.getRatesEnvironmentForAsianOption(dayAfterSettlement, vector<Date>(0));
    BOOST_CHECK(abs(call->getOptionValue(re)) < EPSILON_FOR_DOUBLES);
}

void AsianOptionContactTest::testGetPayoffAfterMaturity()
{
    Date anchorDate(1, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    std::shared_ptr<DayCounter> dayCounter = gr.defaultDayCounter;
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;

    std::shared_ptr<AsianOption> option = getAsianOptionWithMoneynessVol(anchorDate);
    option->setType(PUT);
    std::shared_ptr<AsianOption> option2 = getAsianOptionWithMoneynessVol(anchorDate);
    option2->setType(CALL);

    double v1 = option->getVolume();
    double p1 = option->getPremiumAmount();
    double k1 = option->getStrike();
    double sign1 = (option->getBuySell() == BUY ? 1 : -1);

    double maturitySpot = k1 - 10;
    BOOST_CHECK(abs(option->getPayoff(maturitySpot) - (k1 - maturitySpot - p1) * v1 * sign1) < 1e-14);
    maturitySpot = k1 + 10;
    BOOST_CHECK(abs(option->getPayoff(maturitySpot) - (-p1) * v1 * sign1) < 1e-14);
    BOOST_CHECK(abs(option2->getPayoff(maturitySpot) - (maturitySpot - k1 - p1) * v1 * sign1) < 1e-14);
}

void AsianOptionContactTest::testEuropeanOptionWithMoneynessVol()
{
    BOOST_TEST_MESSAGE("Testing European Option with moneyness vol ...");

    Date anchorDate(1, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    std::shared_ptr<DayCounter> dayCounter = gr.defaultDayCounter;
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;

    std::shared_ptr<AsianOption> call = getAsianOptionWithMoneynessVol(anchorDate);
    std::shared_ptr<AsianOption> callATM = getDefaultAsianOption(anchorDate);

    call->setType(CALL);
    callATM->setType(CALL);

    // 1) Before maturity
    vector<Date> fixingDates, forwardDates;
    call->getFixingAndForwardDates(anchorDate, fixingDates, forwardDates);
    vector<double> forwardRates, fixedRates, sd;
    for (size_t i = 0; i < forwardDates.size(); ++i)
    {
        double tmpFwd = re->getFRS()->getForward(forwardDates[i]);
        forwardRates.push_back(tmpFwd);
        // double moneyness = (call->getStrike() - tmpFwd) / tmpFwd;
        sd.push_back(re->getVRS()->getStandardDeviation(forwardDates[i], call->getStrike(), tmpFwd));
    }
    for (size_t i = 0; i < fixingDates.size(); ++i)
    {
        fixedRates.push_back(re->getFixingRS()->getRateSetOn(fixingDates[i]));
    }
    double strike = call->getStrike();
    double df = re->getDRS()->getDiscountFactor(call->getSettlementDate());
    std::shared_ptr<AverageRateCall> callFormula =
        std::shared_ptr<AverageRateCall>(new AverageRateCall(forwardRates, fixedRates, strike, sd, df));
    double optionFormulaValue =
        (callFormula->getPremium() - call->getPremiumAmount() * df) * call->getVolume();
    BOOST_CHECK(abs(call->getValue(re) - optionFormulaValue) < 1e-10);

    // 2) In Averaging Month
    Date newAnchorDate = nullCalendar->advance(anchorDate, 2, Years);
    newAnchorDate = nullCalendar->advance(anchorDate, 10, Days);
    re = gr.getRatesEnvironmentForAsianOption(newAnchorDate, vector<Date>(0));

    call->getFixingAndForwardDates(anchorDate, fixingDates, forwardDates);
    forwardRates.clear();
    fixedRates.clear();
    sd.clear();
    for (size_t i = 0; i < forwardDates.size(); ++i)
    {
        double tmpFwd = re->getFRS()->getForward(forwardDates[i]);
        forwardRates.push_back(tmpFwd);
        // double moneyness = (call->getStrike() - tmpFwd) / tmpFwd;
        sd.push_back(re->getVRS()->getStandardDeviation(forwardDates[i], call->getStrike(), tmpFwd));
    }
    for (size_t i = 0; i < fixingDates.size(); ++i)
    {
        fixedRates.push_back(re->getFixingRS()->getRateSetOn(fixingDates[i]));
    }
    df = re->getDRS()->getDiscountFactor(call->getSettlementDate());
    callFormula =
        std::shared_ptr<AverageRateCall>(new AverageRateCall(forwardRates, fixedRates, strike, sd, df));
    optionFormulaValue = (callFormula->getPremium() - call->getPremiumAmount() * df) * call->getVolume();
    BOOST_CHECK(abs(call->getValue(re) - optionFormulaValue) < 1e-10);

    // 4) After settlement
    Date dayAfterSettlement = nullCalendar->advance(call->getSettlementDate(), 1, Days);
    re = gr.getRatesEnvironmentForAsianOption(dayAfterSettlement, vector<Date>(0));
    BOOST_CHECK(abs(call->getOptionValue(re)) < EPSILON_FOR_DOUBLES);
}

test_suite* AsianOptionContactTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("AsianOption Contract");

    suite->add(BOOST_TEST_CASE(&AsianOptionContactTest::testMaturityDate));
    suite->add(BOOST_TEST_CASE(&AsianOptionContactTest::testAveragingDates));
    suite->add(BOOST_TEST_CASE(&AsianOptionContactTest::testType));
    suite->add(BOOST_TEST_CASE(&AsianOptionContactTest::testPutCallParity));
    suite->add(BOOST_TEST_CASE(&AsianOptionContactTest::testValue));
    suite->add(BOOST_TEST_CASE(&AsianOptionContactTest::testGetPayoffAfterMaturity));

    suite->add(BOOST_TEST_CASE(&AsianOptionContactTest::testEuropeanOptionWithMoneynessVol));

    return suite;
}
