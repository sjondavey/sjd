#include "CashflowTest.h"

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;

void CashflowTest::testBuySell()
{
    Date anchorDate(1, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 1000;
    BuySell bs = BuySell::BUY;

    std::shared_ptr<FixedCashflow> cf1 =
        GenerateContracts::getDefaultFixedCashflow(anchorDate, yearsToSettlement, volume, bs);
    std::shared_ptr<FixedCashflow> cf2 =
        GenerateContracts::getDefaultFixedCashflow(anchorDate, yearsToSettlement, volume, bs);

    BOOST_CHECK(cf1->getBuySell() == cf2->getBuySell());
    BOOST_CHECK(cf1->getBuySell() == BuySell::BUY);

    std::shared_ptr<FixedCashflow> sold =
        GenerateContracts::getDefaultFixedCashflow(anchorDate, yearsToSettlement, volume, BuySell::SELL);
    BOOST_CHECK(sold->getBuySell() == BuySell::SELL);
}

void CashflowTest::testVolume()
{
    Date anchorDate(1, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 1000;
    BuySell bs = BuySell::BUY;
    std::shared_ptr<FixedCashflow> cf1 =
        GenerateContracts::getDefaultFixedCashflow(anchorDate, yearsToSettlement, volume, bs);
    std::shared_ptr<FixedCashflow> cf2 =
        GenerateContracts::getDefaultFixedCashflow(anchorDate, yearsToSettlement, volume, bs);

    BOOST_CHECK(cf1->isOK());
    BOOST_CHECK(cf2->isOK());
    std::shared_ptr<FixedCashflow> negative =
        GenerateContracts::getDefaultFixedCashflow(anchorDate, yearsToSettlement, -1, bs);
    BOOST_CHECK(negative->isOK());
    BOOST_CHECK(abs(negative->getVolume() - 1.0) < EPSILON_FOR_DOUBLES);
}

void CashflowTest::testMaturityDate()
{
    Date anchorDate(1, Jan, 2013);
    double volume = 1000;
    BuySell bs = BuySell::BUY;
    std::shared_ptr<Calendar> nullCalendar = std::make_shared<NullCalendar>();
    Date maturityDate = nullCalendar->advance(anchorDate, 3, Years);
    FixedCashflow moved(maturityDate, volume, bs);
    BOOST_CHECK(moved.isOK());
    BOOST_CHECK(moved.getMaturityDate() == maturityDate);
    BOOST_CHECK(moved.getSettlementDate() == maturityDate);
}

void CashflowTest::testSettlementDate()
{
    Date anchorDate(1, Jan, 2013);
    double volume = 1000;
    BuySell bs = BuySell::BUY;
    std::shared_ptr<Calendar> nullCalendar = std::make_shared<NullCalendar>();
    Date settlementDate = nullCalendar->advance(anchorDate, 1, Years);
    FixedCashflow moved(settlementDate, volume, bs);
    BOOST_CHECK(moved.isOK());
    BOOST_CHECK(moved.getSettlementDate() == settlementDate);
    BOOST_CHECK(moved.getMaturityDate() == settlementDate);
}

void CashflowTest::testValue()
{
    GenerateRates gr = GenerateRates();

    std::shared_ptr<DayCounter> dayCounter = gr.defaultDayCounter;
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;

    Date anchorDate(1, Jan, 2013);
    Date tPlus2 = nullCalendar->advance(anchorDate, 2, Days);
    Date settlement = nullCalendar->advance(anchorDate, 2, Years);

    vector<Date> manditoryDates;
    manditoryDates.push_back(tPlus2);
    manditoryDates.push_back(settlement);

    [[maybe_unused]] double spot = 100, naccRFR = 0.065;

    std::shared_ptr<DiscountCurve> dc =
        gr.getDiscountCurve(dayCounter, nullCalendar, anchorDate, 10, naccRFR, manditoryDates);
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForCashflow(anchorDate, manditoryDates);

    double volume = 50;
    [[maybe_unused]] double premiumAmount = 45;

    FixedCashflow cf1(settlement, volume, BuySell::BUY);
    FixedCashflow cf2(settlement, volume, BuySell::SELL);

    cf1.validate(re);
    BOOST_REQUIRE(cf1.isOK());
    cf2.validate(re);
    BOOST_REQUIRE(cf2.isOK());

    [[maybe_unused]] double yts = dayCounter->yearFraction(anchorDate, settlement);
    double df = dc->getDiscountFactor(settlement);

    // 1) Before maturity
    BOOST_CHECK(abs(cf1.getValue(re) + cf2.getValue(re)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(cf1.getValue(re) - (volume * df)) / volume < EPSILON_FOR_DOUBLES);

    // 2) After maturity with ContractStatus = Excecised

    // 3) After settlement
    Date dayAfterSettlement = nullCalendar->advance(settlement, 1, Days);
    manditoryDates.clear();
    re = gr.getRatesEnvironmentForCashflow(dayAfterSettlement, manditoryDates);
    BOOST_CHECK(abs(cf2.getValue(re)) < EPSILON_FOR_DOUBLES);
}

void CashflowTest::testGetPayoffAfterMaturity()
{
    std::shared_ptr<DayCounter> dayCounter = std::make_shared<Actual365Fixed>();
    std::shared_ptr<Calendar> nullCalendar = std::make_shared<NullCalendar>();

    Date anchorDate(1, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 1000;
    BuySell bs = BuySell::BUY;
    std::shared_ptr<FixedCashflow> cf1 =
        GenerateContracts::getDefaultFixedCashflow(anchorDate, yearsToSettlement, volume, bs);
    std::shared_ptr<FixedCashflow> cf2 =
        GenerateContracts::getDefaultFixedCashflow(anchorDate, yearsToSettlement, volume, bs);

    double v1 = cf1->getVolume();
    double v2 = cf2->getVolume();
    double sign1 = (cf1->getBuySell() == BuySell::BUY ? 1 : -1);
    double sign2 = (cf2->getBuySell() == BuySell::BUY ? 1 : -1);

    BOOST_CHECK(cf1->getPayoff(0) == (v1)*sign1);
    BOOST_CHECK(cf2->getPayoff(0) == (v2)*sign2);
}

test_suite* CashflowTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("FixedCashFlows");

    suite->add(BOOST_TEST_CASE(&CashflowTest::testBuySell));
    suite->add(BOOST_TEST_CASE(&CashflowTest::testVolume));
    suite->add(BOOST_TEST_CASE(&CashflowTest::testMaturityDate));
    suite->add(BOOST_TEST_CASE(&CashflowTest::testSettlementDate));

    suite->add(BOOST_TEST_CASE(&CashflowTest::testValue));
    suite->add(BOOST_TEST_CASE(&CashflowTest::testGetPayoffAfterMaturity));

    return suite;
}
