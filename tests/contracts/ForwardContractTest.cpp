#include "ForwardContractTest.h"

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;

std::shared_ptr<EuropeanForward> ForwardContractTest::getDefaultEuropeanForward(Date anchorDate)
{
    NullCalendar nullCalendar;
    Date rateSetDate = nullCalendar.advance(anchorDate, 2, Years);
    Date settlement = nullCalendar.advance(rateSetDate, 2, Days);

    double strike = 100.0;
    double volume = 150;
    return std::make_shared<EuropeanForward>(rateSetDate, settlement, strike, volume, BuySell::BUY);
}

std::shared_ptr<AsianForward> ForwardContractTest::getDefaultAsianForward(Date anchorDate)
{
    NullCalendar nullCalendar;
    Date rateSetDate = nullCalendar.advance(anchorDate, 2, Years);
    set<Date> averagingDates;
    Date d = Date(1, rateSetDate.month(), rateSetDate.year());
    Date nextMonth = nullCalendar.advance(d, 1, Months);
    while (d < nextMonth)
    {
        averagingDates.insert(d);
        d = nullCalendar.advance(d, 1, Days);
    }
    vector<Date> averagingDatesAsVector = vector<Date>(averagingDates.begin(), averagingDates.end());
    Date settlement = nullCalendar.advance(averagingDatesAsVector.back(), 2, Days);

    double strike = 100;
    double volume = 50;

    return std::make_shared<AsianForward>(averagingDates, settlement, strike, volume, BuySell::BUY);
}

void ForwardContractTest::testBuySell()
{
    BOOST_TEST_MESSAGE("Testing ForwardContract ...");

    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> base = getDefaultEuropeanForward(anchorDate);
    EuropeanForward sold(base->getMaturityDate(), base->getSettlementDate(), base->getStrike(),
                         base->getVolume(), BuySell::SELL);
    BOOST_CHECK(sold.getBuySell() == BuySell::SELL);
}

void ForwardContractTest::testStrike()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> contract = getDefaultEuropeanForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    contract->validate();
    BOOST_CHECK(contract->isOK());
    EuropeanForward negative(contract->getMaturityDate(), contract->getSettlementDate(), -1.0,
                             contract->getVolume(), contract->getBuySell());
    negative.validate();
    BOOST_CHECK(!negative.isOK());
    BOOST_CHECK(abs(negative.getStrike() + 1.0) < EPSILON_FOR_DOUBLES);
}

void ForwardContractTest::testVolume()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> contract = getDefaultEuropeanForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    contract->validate();
    BOOST_CHECK(contract->isOK());
    EuropeanForward negative(contract->getMaturityDate(), contract->getSettlementDate(),
                             contract->getStrike(), -1.0, contract->getBuySell());
    negative.validate();
    BOOST_CHECK(negative.isOK());
    BOOST_CHECK(abs(negative.getVolume() - 1.0) < EPSILON_FOR_DOUBLES);
}

void ForwardContractTest::testMaturityDate()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> contract = getDefaultEuropeanForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    contract->validate();
    BOOST_CHECK(contract->isOK());
    NullCalendar nullCalendar;
    Date maturityDate = nullCalendar.advance(anchorDate, 3, Years);
    EuropeanForward later(maturityDate, contract->getSettlementDate(), contract->getStrike(),
                          contract->getVolume(), contract->getBuySell());
    later.validate();
    BOOST_CHECK(!later.isOK());
    BOOST_CHECK(later.getMaturityDate() == maturityDate);
}

void ForwardContractTest::testSettlementDate()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> contract = getDefaultEuropeanForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    contract->validate();
    BOOST_CHECK(contract->isOK());
    NullCalendar nullCalendar;
    Date settlementDate = nullCalendar.advance(anchorDate, 1, Years);
    EuropeanForward earlier(contract->getMaturityDate(), settlementDate, contract->getStrike(),
                            contract->getVolume(), contract->getBuySell());
    earlier.validate();
    BOOST_CHECK(!earlier.isOK());
    BOOST_CHECK(earlier.getSettlementDate() == settlementDate);
}

void ForwardContractTest::testValidationReport()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanForward> contract = getDefaultEuropeanForward(anchorDate);

    ValidationReport snapshot = contract->validationReport();
    BOOST_CHECK(snapshot.isOK());
    BOOST_CHECK(snapshot.getIssues().empty());

    EuropeanForward invalid(contract->getMaturityDate(), contract->getSettlementDate(), -1.0,
                            contract->getVolume(), contract->getBuySell());
    contract = std::make_shared<EuropeanForward>(invalid);
    ValidationReport invalidReport = contract->validateReport();
    BOOST_REQUIRE(!invalidReport.isOK());
    BOOST_REQUIRE(invalidReport.getIssues().size() == 1);
    BOOST_CHECK(invalidReport.getIssues()[0].code == "CONTRACT_STRIKE_NEGATIVE");
    BOOST_REQUIRE(invalidReport.getIssues()[0].path.size() == 2);
    BOOST_CHECK(invalidReport.getIssues()[0].path[0] == "european_forward");
    BOOST_CHECK(invalidReport.getIssues()[0].path[1] == "strike");
    BOOST_CHECK(invalidReport.getIssues()[0].message == "Strike is negative");
    BOOST_CHECK(contract->getErrorMessagesAsString() == "EuropeanForward: Strike is negative");

    EuropeanForward corrected(contract->getMaturityDate(), contract->getSettlementDate(), 100.0,
                              contract->getVolume(), contract->getBuySell());
    ValidationReport correctedReport = corrected.validateReport();
    BOOST_CHECK(correctedReport.isOK());
    BOOST_CHECK(correctedReport.getIssues().empty());
    BOOST_CHECK(corrected.getErrorMessages().empty());

    std::shared_ptr<RatesEnvironment> emptyEnvironment(std::make_shared<RatesEnvironment>(anchorDate));
    ValidationReport marketReport = corrected.validateReport(emptyEnvironment);
    BOOST_REQUIRE(!marketReport.isOK());
    BOOST_REQUIRE(marketReport.getIssues().size() == 3);
    BOOST_CHECK(marketReport.getIssues()[0].code == "RATES_ENVIRONMENT_NO_RATES");
    BOOST_CHECK(marketReport.getIssues()[0].path[0] == "european_forward");
    BOOST_CHECK(marketReport.getIssues()[0].path[1] == "rates_environment");
    BOOST_CHECK(marketReport.getIssues()[1].code == "MARKET_DISCOUNT_RATE_SOURCE_MISSING");
    BOOST_CHECK(marketReport.getIssues()[2].code == "MARKET_FORWARD_RATE_SOURCE_MISSING");
}

void ForwardContractTest::testValue()
{

    std::shared_ptr<RatesEnvironment> re;
    GenerateRates gr = GenerateRates();

    DayCounter dayCounter = gr.defaultDayCounter;
    Calendar nullCalendar = gr.defaultCalendar;

    Date anchorDate(1, Jan, 2013);
    Date tPlus2 = nullCalendar.advance(anchorDate, 2, Days);
    Date rateSetDate = nullCalendar.advance(anchorDate, 2, Years);
    Date settlement = nullCalendar.advance(rateSetDate, 2, Days);

    vector<Date> manditoryDates;
    manditoryDates.push_back(tPlus2);
    manditoryDates.push_back(rateSetDate);
    manditoryDates.push_back(settlement);

    [[maybe_unused]] double spot = 100;
    [[maybe_unused]] double naccRFR = 0.065;
    [[maybe_unused]] double naccIRDiff = 0.045;
    [[maybe_unused]] double vol = 0.2;

    std::shared_ptr<DiscountCurve> dc =
        gr.getDiscountCurve(dayCounter, nullCalendar, anchorDate, 10, naccRFR, manditoryDates);
    std::shared_ptr<ForwardCurveDeterministic> fc =
        gr.getSimpleForwardCurve(dayCounter, nullCalendar, anchorDate, 10, spot, naccIRDiff, manditoryDates);
    re = std::make_shared<RatesEnvironment>(anchorDate);
    re->setDRS(dc);
    re->setFRS(fc);

    double strike = 100;
    double volume = 50;

    EuropeanForward contract(rateSetDate, settlement, strike, volume, BuySell::BUY);
    EuropeanForward contract2(rateSetDate, settlement, strike, volume, BuySell::SELL);

    contract.validate(re);
    BOOST_REQUIRE(contract.isOK());
    contract2.validate(re);
    BOOST_REQUIRE(contract2.isOK());

    [[maybe_unused]] double yte = dayCounter.yearFraction(anchorDate, rateSetDate);
    [[maybe_unused]] double yts = dayCounter.yearFraction(anchorDate, settlement);
    double F = fc->getForward(settlement);
    double df = dc->getDiscountFactor(settlement);

    // 1) Before maturity
    BOOST_CHECK(abs(contract.getValue(re) + contract2.getValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(contract.getValue(re) - volume * (F - strike) * df) / volume < EPSILON_FOR_DOUBLES);

    // 3) After settlement
    Date dayAfterSettlement = nullCalendar.advance(settlement, 1, Days);
    manditoryDates.clear();
    dc = gr.getDiscountCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, naccRFR);
    fc = gr.getSimpleForwardCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, spot, naccIRDiff);
    re = std::make_shared<RatesEnvironment>(dayAfterSettlement);
    re->setDRS(dc);
    re->setFRS(fc);

    BOOST_CHECK(abs(contract2.getValue(re)) < EPSILON_FOR_DOUBLES);
}

void ForwardContractTest::testGetPayoffAfterMaturity()
{
    Actual365Fixed dayCounter;
    NullCalendar nullCalendar;

    Date anchorDate(1, Jan, 2013);

    std::shared_ptr<EuropeanForward> fwd = getDefaultEuropeanForward(anchorDate);

    double v1 = fwd->getVolume();
    double p1 = fwd->getPremiumAmount();
    double k1 = fwd->getStrike();
    double sign1 = (fwd->getBuySell() == BuySell::BUY ? 1 : -1);

    double maturitySpot = k1 + 10;
    BOOST_CHECK(abs(fwd->getPayoff(maturitySpot) - (maturitySpot - k1 - p1) * v1 * sign1) < 1e-14);
}

void ForwardContractTest::testAsianAveragingDates()
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<AsianForward> contract = getDefaultAsianForward(anchorDate);
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    std::shared_ptr<DailyFixingCurve> fixingCurve, fixingCurve2;
    BOOST_CHECK(contract->isOK());

    NullCalendar nullCalendar;
    Date rateSetDate = nullCalendar.advance(anchorDate, 3, Years); // after settlement
    set<Date> averagingDates;
    Date d = Date(1, rateSetDate.month(), rateSetDate.year());
    Date nextMonth = nullCalendar.advance(d, 1, Months);
    while (d < nextMonth)
    {
        averagingDates.insert(d);
        d = nullCalendar.advance(d, 1, Days);
    }
    contract =
        std::make_shared<AsianForward>(averagingDates, contract->getSettlementDate(), contract->getStrike(),
                                       contract->getVolume(), contract->getBuySell());
    contract->validate();
    BOOST_CHECK(!contract->isOK());
}

void ForwardContractTest::testFixingAndForwardDates()
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

void ForwardContractTest::testAsianForward()
{

    std::shared_ptr<RatesEnvironment> re;
    GenerateRates gr = GenerateRates();

    DayCounter dayCounter = gr.defaultDayCounter;
    NullCalendar nullCalendar;

    Date anchorDate(1, Jan, 2013);
    Date tPlus2 = nullCalendar.advance(anchorDate, 2, Days);

    set<Date> averagingDates;
    vector<Date> manditoryDates;
    manditoryDates.push_back(tPlus2);
    Date d = Date(1, Jan, 2014);
    Date nextMonth = Date(1, Feb, 2014);
    while (d < nextMonth)
    {
        averagingDates.insert(d);
        manditoryDates.push_back(d);
        d = nullCalendar.advance(d, 1, Days);
    }
    vector<Date> averagingDatesAsVector = vector<Date>(averagingDates.begin(), averagingDates.end());
    Date settlement = nullCalendar.advance(averagingDatesAsVector.back(), 2, Days);
    manditoryDates.push_back(settlement);

    [[maybe_unused]] double spot = 100;
    [[maybe_unused]] double naccRFR = 0.065;
    [[maybe_unused]] double naccIRDiff = 0.045;
    [[maybe_unused]] double vol = 0.2;

    std::shared_ptr<DiscountCurve> dc =
        gr.getDiscountCurve(dayCounter, nullCalendar, anchorDate, 10, naccRFR, manditoryDates);
    std::shared_ptr<ForwardCurveDeterministic> fc =
        gr.getSimpleForwardCurve(dayCounter, nullCalendar, anchorDate, 10, spot, naccIRDiff, manditoryDates);
    std::shared_ptr<DailyFixingCurve> fixingCurve = gr.buildFixingCurve(90.0, averagingDatesAsVector);
    re = std::make_shared<RatesEnvironment>(anchorDate);
    re->setDRS(dc);
    re->setFRS(fc);
    re->setFixingRS(fixingCurve);

    double strike = 100;
    double volume = 50;

    AsianForward contract(averagingDates, settlement, strike, volume, BuySell::BUY);
    AsianForward contract2(averagingDates, settlement, strike, volume, BuySell::SELL);

    contract.validate(re);
    BOOST_REQUIRE(contract.isOK());
    contract2.validate(re);
    BOOST_REQUIRE(contract2.isOK());

    // double yte = dayCounter.yearFraction(anchorDate, rateSetDate);
    [[maybe_unused]] double yts = dayCounter.yearFraction(anchorDate, settlement);
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
    Date dayAfterSettlement = nullCalendar.advance(settlement, 1, Days);
    manditoryDates.clear();
    dc = gr.getDiscountCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, naccRFR);
    fc = gr.getSimpleForwardCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, spot, naccIRDiff);
    re = std::make_shared<RatesEnvironment>(dayAfterSettlement);
    re->setDRS(dc);
    re->setFRS(fc);
    BOOST_CHECK(abs(contract2.getValue(re)) < EPSILON_FOR_DOUBLES);
}

void ForwardContractTest::testAsianGetPayoffAfterMaturity()
{
    Actual365Fixed dayCounter;
    NullCalendar nullCalendar;

    Date anchorDate(1, Jan, 2013);

    std::shared_ptr<AsianForward> fwd = getDefaultAsianForward(anchorDate);

    double v1 = fwd->getVolume();
    double p1 = fwd->getPremiumAmount();
    double k1 = fwd->getStrike();
    double sign1 = (fwd->getBuySell() == BuySell::BUY ? 1 : -1);

    double maturitySpot = k1 + 10;
    BOOST_CHECK(abs(fwd->getPayoff(maturitySpot) - (maturitySpot - k1 - p1) * v1 * sign1) < 1e-14);
}

test_suite* ForwardContractTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("Forward Contracts");

    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testBuySell));
    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testStrike));
    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testVolume));
    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testMaturityDate));
    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testSettlementDate));
    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testValidationReport));
    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testValue));
    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testGetPayoffAfterMaturity));

    BOOST_TEST_MESSAGE("Testing AsianForwards ...");
    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testAsianAveragingDates));
    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testFixingAndForwardDates));

    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testAsianForward));
    suite->add(BOOST_TEST_CASE(&ForwardContractTest::testAsianGetPayoffAfterMaturity));

    return suite;
}
