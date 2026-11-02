#include "ForwardContractTest.h"

using namespace std;
using namespace QuantLib;
using namespace boost;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;

std::shared_ptr<EuropeanForward> ForwardContactTest::getDefaultEuropeanForward(Date anchorDate)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date rateSetDate = nullCalendar->advance(anchorDate, 2, Years);
    Date settlement = nullCalendar->advance(rateSetDate, 2, Days);

    double strike = 100.0;
    double volume = 150;
    return std::shared_ptr<EuropeanForward>(
        new EuropeanForward(rateSetDate, settlement, strike, volume, BUY));
}

std::shared_ptr<AsianForward> ForwardContactTest::getDefaultAsianForward(Date anchorDate)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date rateSetDate = nullCalendar->advance(anchorDate, 2, Years);
    set<Date> averagingDates;
    Date d = Date(1, rateSetDate.month(), rateSetDate.year());
    Date nextMonth = nullCalendar->advance(d, 1, Months);
    while (d < nextMonth)
    {
        averagingDates.insert(d);
        d = nullCalendar->advance(d, 1, Days);
    }
    vector<Date> averagingDatesAsVector = vector<Date>(averagingDates.begin(), averagingDates.end());
    Date settlement = nullCalendar->advance(averagingDatesAsVector.back(), 2, Days);

    double strike = 100;
    double volume = 50;

    return std::shared_ptr<AsianForward>(new AsianForward(averagingDates, settlement, strike, volume, BUY));
}

void ForwardContactTest::testBuySell()
{
    BOOST_TEST_MESSAGE("Testing ForwardContract ...");

    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> fc = getDefaultEuropeanForward(anchorDate);
    BuySell bs = SELL;
    fc->setBuySell(bs);
    BuySell bs2 = fc->getBuySell();
    BOOST_CHECK(bs == bs2);
}

void ForwardContactTest::testStrike()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> contract = getDefaultEuropeanForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    contract->validate();
    BOOST_CHECK(contract->isOK());
    double strike = -1;
    contract->setStrike(strike);
    contract->validate();
    BOOST_CHECK(!contract->isOK());
    BOOST_CHECK(abs(contract->getStrike() - strike) < EPSILON_FOR_DOUBLES);
}

void ForwardContactTest::testVolume()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> contract = getDefaultEuropeanForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    contract->validate();
    BOOST_CHECK(contract->isOK());
    double volume = -1;
    contract->setVolume(volume);
    contract->validate();
    BOOST_CHECK(contract->isOK());
    BOOST_CHECK(abs(contract->getVolume() + volume) < EPSILON_FOR_DOUBLES);
}

void ForwardContactTest::testMaturityDate()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> contract = getDefaultEuropeanForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    contract->validate();
    BOOST_CHECK(contract->isOK());
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date maturityDate = nullCalendar->advance(anchorDate, 3, Years);
    contract->setMaturityDate(maturityDate);
    contract->validate();
    BOOST_CHECK(!contract->isOK());
    BOOST_CHECK(contract->getMaturityDate() == maturityDate);
}

void ForwardContactTest::testSettlementDate()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> contract = getDefaultEuropeanForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    contract->validate();
    BOOST_CHECK(contract->isOK());
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date settlementDate = nullCalendar->advance(anchorDate, 1, Years);
    contract->setSettlementDate(settlementDate);
    contract->validate();
    BOOST_CHECK(!contract->isOK());
    BOOST_CHECK(contract->getSettlementDate() == settlementDate);
}

void ForwardContactTest::testValidationReport()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> contract = getDefaultEuropeanForward(anchorDate);

    // A snapshot does not validate or mutate the contract.
    contract->setStrike(-1.0);
    ValidationReport snapshot = contract->validationReport();
    BOOST_CHECK(snapshot.isOK());
    BOOST_CHECK(snapshot.getIssues().empty());

    ValidationReport invalidReport = contract->validateReport();
    BOOST_REQUIRE(!invalidReport.isOK());
    BOOST_REQUIRE(invalidReport.getIssues().size() == 1);
    BOOST_CHECK(invalidReport.getIssues()[0].code == "CONTRACT_STRIKE_NEGATIVE");
    BOOST_REQUIRE(invalidReport.getIssues()[0].path.size() == 2);
    BOOST_CHECK(invalidReport.getIssues()[0].path[0] == "european_forward");
    BOOST_CHECK(invalidReport.getIssues()[0].path[1] == "strike");
    BOOST_CHECK(invalidReport.getIssues()[0].message == "Strike is negative");
    BOOST_CHECK(contract->getErrorMessagesAsString() == "EuropeanForward: Strike is negative");

    // Revalidation after correcting the input clears stale issues.
    contract->setStrike(100.0);
    ValidationReport correctedReport = contract->validateReport();
    BOOST_CHECK(correctedReport.isOK());
    BOOST_CHECK(correctedReport.getIssues().empty());
    BOOST_CHECK(contract->getErrorMessages().empty());

    std::shared_ptr<RatesEnvironment> emptyEnvironment(new RatesEnvironment(anchorDate));
    ValidationReport marketReport = contract->validateReport(emptyEnvironment);
    BOOST_REQUIRE(!marketReport.isOK());
    BOOST_REQUIRE(marketReport.getIssues().size() == 3);
    BOOST_CHECK(marketReport.getIssues()[0].code == "RATES_ENVIRONMENT_NO_RATES");
    BOOST_CHECK(marketReport.getIssues()[0].path[0] == "european_forward");
    BOOST_CHECK(marketReport.getIssues()[0].path[1] == "rates_environment");
    BOOST_CHECK(marketReport.getIssues()[1].code == "MARKET_DISCOUNT_RATE_SOURCE_MISSING");
    BOOST_CHECK(marketReport.getIssues()[2].code == "MARKET_FORWARD_RATE_SOURCE_MISSING");
}

void ForwardContactTest::testValue()
{

    std::shared_ptr<RatesEnvironment> re;
    GenerateRates gr = GenerateRates();

    std::shared_ptr<DayCounter> dayCounter = gr.defaultDayCounter;
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;

    Date anchorDate(1, Jan, 2013);
    Date tPlus2 = nullCalendar->advance(anchorDate, 2, Days);
    Date rateSetDate = nullCalendar->advance(anchorDate, 2, Years);
    Date settlement = nullCalendar->advance(rateSetDate, 2, Days);

    vector<Date> manditoryDates;
    manditoryDates.push_back(tPlus2);
    manditoryDates.push_back(rateSetDate);
    manditoryDates.push_back(settlement);

    double spot = 100, naccRFR = 0.065, naccIRDiff = 0.045, vol = 0.2;

    std::shared_ptr<DiscountCurve> dc =
        gr.getDiscountCurve(dayCounter, nullCalendar, anchorDate, 10, naccRFR, manditoryDates);
    std::shared_ptr<ForwardCurveDeterministic> fc =
        gr.getSimpleForwardCurve(dayCounter, nullCalendar, anchorDate, 10, spot, naccIRDiff, manditoryDates);
    re = std::shared_ptr<RatesEnvironment>(new RatesEnvironment(anchorDate));
    re->setDRS(dc);
    re->setFRS(fc);

    double strike = 100;
    double volume = 50;

    EuropeanForward contract(rateSetDate, settlement, strike, volume, BUY);
    EuropeanForward contract2(contract);
    contract2.setBuySell(SELL);

    contract.validate(re);
    BOOST_REQUIRE(contract.isOK());
    contract2.validate(re);
    BOOST_REQUIRE(contract2.isOK());

    double yte = dayCounter->yearFraction(anchorDate, rateSetDate);
    double yts = dayCounter->yearFraction(anchorDate, settlement);
    double F = fc->getForward(settlement);
    double df = dc->getDiscountFactor(settlement);

    // 1) Before maturity
    BOOST_CHECK(abs(contract.getValue(re) + contract2.getValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(contract.getValue(re) - volume * (F - strike) * df) / volume < EPSILON_FOR_DOUBLES);

    // 3) After settlement
    Date dayAfterSettlement = nullCalendar->advance(settlement, 1, Days);
    manditoryDates.clear();
    dc = gr.getDiscountCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, naccRFR);
    fc = gr.getSimpleForwardCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, spot, naccIRDiff);
    re = std::shared_ptr<RatesEnvironment>(new RatesEnvironment(dayAfterSettlement));
    re->setDRS(dc);
    re->setFRS(fc);

    BOOST_CHECK(abs(contract2.getValue(re)) < EPSILON_FOR_DOUBLES);
}

void ForwardContactTest::testGetPayoffAfterMaturity()
{
    std::shared_ptr<DayCounter> dayCounter = std::shared_ptr<DayCounter>(new Actual365Fixed());
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());

    Date anchorDate(1, Jan, 2013);

    std::shared_ptr<EuropeanForward> fwd = getDefaultEuropeanForward(anchorDate);

    double v1 = fwd->getVolume();
    double p1 = fwd->getPremiumAmount();
    double k1 = fwd->getStrike();
    double sign1 = (fwd->getBuySell() == BUY ? 1 : -1);

    double maturitySpot = k1 + 10;
    BOOST_CHECK(abs(fwd->getPayoff(maturitySpot) - (maturitySpot - k1 - p1) * v1 * sign1) < 1e-14);
}

void ForwardContactTest::testAsianAveragingDates()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<AsianForward> contract = getDefaultAsianForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    std::shared_ptr<DailyFixingCurve> fixingCurve, fixingCurve2;
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
    BOOST_CHECK(!contract->isOK());
}

void ForwardContactTest::testFixingAndForwardDates()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<AsianForward> contract = getDefaultAsianForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    std::shared_ptr<DailyFixingCurve> fixingCurve, fixingCurve2;
    BOOST_CHECK(contract->isOK());

    vector<Date> fixingDates, forwardDates, averagingDates;
    averagingDates = contract->getAveragingDates();

    size_t i = averagingDates.size() / 2;
    Date valuationDate = averagingDates[i];

    contract->getFixingAndForwardDates(valuationDate, fixingDates, forwardDates);
    BOOST_CHECK(fixingDates.size() == i + 1);
    BOOST_CHECK(forwardDates.size() == averagingDates.size() - i - 1);
}

void ForwardContactTest::testAsianForward()
{

    std::shared_ptr<RatesEnvironment> re;
    GenerateRates gr = GenerateRates();

    std::shared_ptr<DayCounter> dayCounter = gr.defaultDayCounter;
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());

    Date anchorDate(1, Jan, 2013);
    Date tPlus2 = nullCalendar->advance(anchorDate, 2, Days);

    set<Date> averagingDates;
    vector<Date> manditoryDates;
    manditoryDates.push_back(tPlus2);
    Date d = Date(1, Jan, 2014);
    Date nextMonth = Date(1, Feb, 2014);
    while (d < nextMonth)
    {
        averagingDates.insert(d);
        manditoryDates.push_back(d);
        d = nullCalendar->advance(d, 1, Days);
    }
    vector<Date> averagingDatesAsVector = vector<Date>(averagingDates.begin(), averagingDates.end());
    Date settlement = nullCalendar->advance(averagingDatesAsVector.back(), 2, Days);
    manditoryDates.push_back(settlement);

    double spot = 100, naccRFR = 0.065, naccIRDiff = 0.045, vol = 0.2;

    std::shared_ptr<DiscountCurve> dc =
        gr.getDiscountCurve(dayCounter, nullCalendar, anchorDate, 10, naccRFR, manditoryDates);
    std::shared_ptr<ForwardCurveDeterministic> fc =
        gr.getSimpleForwardCurve(dayCounter, nullCalendar, anchorDate, 10, spot, naccIRDiff, manditoryDates);
    std::shared_ptr<DailyFixingCurve> fixingCurve = gr.buildFixingCurve(90.0, averagingDatesAsVector);
    re = std::shared_ptr<RatesEnvironment>(new RatesEnvironment(anchorDate));
    re->setDRS(dc);
    re->setFRS(fc);
    re->setFixingRS(fixingCurve);

    double strike = 100;
    double volume = 50;

    AsianForward contract(averagingDates, settlement, strike, volume, BUY);
    AsianForward contract2(contract);
    contract2.setBuySell(SELL);

    contract.validate(re);
    BOOST_REQUIRE(contract.isOK());
    contract2.validate(re);
    BOOST_REQUIRE(contract2.isOK());

    // double yte = dayCounter->yearFraction(anchorDate, rateSetDate);
    double yts = dayCounter->yearFraction(anchorDate, settlement);
    double averageFwd = 0;
    for (size_t i = 0; i < averagingDatesAsVector.size(); ++i)
    {
        averageFwd += fc->getForward(averagingDatesAsVector[i]);
    }
    averageFwd /= averagingDatesAsVector.size();

    double df = dc->getDiscountFactor(settlement);

    // 1) Before maturity
    BOOST_CHECK(abs(contract.getValue(re) + contract2.getValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(contract.getValue(re) - volume * (averageFwd - strike) * df) / volume <
                EPSILON_FOR_DOUBLES);

    // 3) After settlement
    Date dayAfterSettlement = nullCalendar->advance(settlement, 1, Days);
    manditoryDates.clear();
    dc = gr.getDiscountCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, naccRFR);
    fc = gr.getSimpleForwardCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, spot, naccIRDiff);
    re = std::shared_ptr<RatesEnvironment>(new RatesEnvironment(dayAfterSettlement));
    re->setDRS(dc);
    re->setFRS(fc);
    BOOST_CHECK(abs(contract2.getValue(re)) < EPSILON_FOR_DOUBLES);
}

void ForwardContactTest::testAsianGetPayoffAfterMaturity()
{
    std::shared_ptr<DayCounter> dayCounter = std::shared_ptr<DayCounter>(new Actual365Fixed());
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());

    Date anchorDate(1, Jan, 2013);

    std::shared_ptr<AsianForward> fwd = getDefaultAsianForward(anchorDate);

    double v1 = fwd->getVolume();
    double p1 = fwd->getPremiumAmount();
    double k1 = fwd->getStrike();
    double sign1 = (fwd->getBuySell() == BUY ? 1 : -1);

    double maturitySpot = k1 + 10;
    BOOST_CHECK(abs(fwd->getPayoff(maturitySpot) - (maturitySpot - k1 - p1) * v1 * sign1) < 1e-14);
}

test_suite* ForwardContactTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("Forward Contracts");

    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testBuySell));
    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testStrike));
    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testVolume));
    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testMaturityDate));
    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testSettlementDate));
    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testValidationReport));
    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testValue));
    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testGetPayoffAfterMaturity));

    BOOST_TEST_MESSAGE("Testing AsianForwards ...");
    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testAsianAveragingDates));
    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testFixingAndForwardDates));

    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testAsianForward));
    suite->add(BOOST_TEST_CASE(&ForwardContactTest::testAsianGetPayoffAfterMaturity));

    return suite;
}
