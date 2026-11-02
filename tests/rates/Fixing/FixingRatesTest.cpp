#include "FixingRatesTest.h"

using namespace std;
using namespace QuantLib;
using namespace boost;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

void FixingRateTest::testDailyFixingCurve()
{
    BOOST_TEST_MESSAGE("Testing Daily Fixing Curve ...");

    vector<Date> observationDates;

    Date anchorDate = Date(01, Oct, 2014);
    for (size_t i = 0; i < 10; ++i)
    {
        observationDates.push_back(anchorDate + i);
    }

    static const double arr[] = {100.50, 100.30, 100.10, 102.00, 101.00,
                                 101.50, 102.00, 103.20, 101.10, 101.05};
    vector<double> rates(arr, arr + sizeof(arr) / sizeof(arr[0]));

    Date startDate = observationDates.front();
    Date endDate = observationDates.back();

    std::shared_ptr<DailyFixingCurve> fix =
        std::shared_ptr<DailyFixingCurve>(new DailyFixingCurve(observationDates, rates));

    BOOST_REQUIRE(fix->isOK());
    BOOST_CHECK(startDate == fix->getEarliestDate());
    BOOST_CHECK(endDate == fix->getMostRecentDate());
    BOOST_CHECK(fix->containsARateSetOn(observationDates[2]));
    BOOST_CHECK(!fix->containsARateSetOn(anchorDate - 1));
    BOOST_CHECK(abs(fix->getRateSetOn(observationDates[3]) - rates[3]) < EPSILON_FOR_DOUBLES);

    rates.push_back(100);
    fix = std::shared_ptr<DailyFixingCurve>(new DailyFixingCurve(observationDates, rates));
    BOOST_CHECK(!fix->isOK());

    observationDates.push_back(anchorDate - 1);
    fix = std::shared_ptr<DailyFixingCurve>(new DailyFixingCurve(observationDates, rates));
    BOOST_CHECK(!fix->isOK());

    observationDates.pop_back();
    rates.pop_back();
    fix = std::shared_ptr<DailyFixingCurve>(new DailyFixingCurve(observationDates, rates));
    BOOST_REQUIRE(fix->isOK());

    vector<Date> appendedDates;
    for (size_t i = 1; i <= 10; ++i)
    {
        appendedDates.push_back(endDate + i);
    }
    vector<double> appendedRates(arr, arr + sizeof(arr) / sizeof(arr[0]));
    fix->appendRateSets(appendedDates, appendedRates);
    BOOST_CHECK(fix->isOK());
    BOOST_CHECK(appendedDates.back() == fix->getMostRecentDate());
    BOOST_CHECK(fix->containsARateSetOn(appendedDates[2]));

    fix->reset();
    BOOST_CHECK(fix->isOK());
    BOOST_CHECK(endDate == fix->getMostRecentDate());
    BOOST_CHECK(!fix->containsARateSetOn(appendedDates[2]));
}

void FixingRateTest::testMonthlyFixingCurve()
{
    BOOST_TEST_MESSAGE("Testing Monthly Fixing Curve ...");

    vector<Date> observationDates;

    Date anchorDate = Date(01, Oct, 2014);
    Calendar nullCalendar = NullCalendar();
    for (size_t i = 0; i < 10; ++i)
    {
        observationDates.push_back(nullCalendar.advance(anchorDate, i, Months));
    }

    static const double arr[] = {100.50, 100.30, 100.10, 102.00, 101.00,
                                 101.50, 102.00, 103.20, 101.10, 101.05};
    vector<double> rates(arr, arr + sizeof(arr) / sizeof(arr[0]));

    Date startDate = observationDates.front();
    Date endDate = observationDates.back();

    std::shared_ptr<MonthlyFixingCurve> fix =
        std::shared_ptr<MonthlyFixingCurve>(new MonthlyFixingCurve(observationDates, rates));

    BOOST_REQUIRE(fix->isOK());
    BOOST_CHECK(startDate == fix->getEarliestDate());
    BOOST_CHECK(endDate == fix->getMostRecentDate());
    BOOST_CHECK(fix->containsARateSetOn(observationDates[2]));
    BOOST_CHECK(!fix->containsARateSetOn(anchorDate - 1));
    BOOST_CHECK(abs(fix->getRateSetOn(observationDates[3]) - rates[3]) < EPSILON_FOR_DOUBLES);
    Date d1 = observationDates[3];
    Date d2 = d1 + 1;
    BOOST_CHECK(fix->containsARateSetOn(d2));
    BOOST_CHECK(abs(fix->getRateSetOn(d1) - fix->getRateSetOn(d2)) < EPSILON_FOR_DOUBLES);

    rates.push_back(100);
    fix = std::shared_ptr<MonthlyFixingCurve>(new MonthlyFixingCurve(observationDates, rates));
    BOOST_CHECK(!fix->isOK());

    observationDates.push_back(anchorDate - 1);
    fix = std::shared_ptr<MonthlyFixingCurve>(new MonthlyFixingCurve(observationDates, rates));
    BOOST_CHECK(!fix->isOK());

    observationDates.pop_back();
    rates.pop_back();
    vector<Date> appendedDates;
    for (size_t i = 1; i <= 10; ++i)
    {
        appendedDates.push_back(nullCalendar.advance(endDate, i, Months));
    }
    vector<double> appendedRates(arr, arr + sizeof(arr) / sizeof(arr[0]));
    fix = std::shared_ptr<MonthlyFixingCurve>(new MonthlyFixingCurve(observationDates, rates));
    BOOST_CHECK(fix->isOK());
    fix->appendRateSets(appendedDates, appendedRates);
    BOOST_CHECK(fix->isOK());
    BOOST_CHECK(appendedDates.back() == fix->getMostRecentDate());
    BOOST_CHECK(fix->containsARateSetOn(appendedDates[2]));

    fix->reset();
    BOOST_CHECK(fix->isOK());
    BOOST_CHECK(endDate == fix->getMostRecentDate());
    BOOST_CHECK(!fix->containsARateSetOn(appendedDates[2]));
}

void FixingRateTest::testHistoricFedFundsCurve()
{
    BOOST_TEST_MESSAGE("Testing HistoricFedFundsCurve ...");

    vector<Date> observationDates;

    Date observationDate = Date(01, Oct, 2014);
    for (size_t i = 0; i < 31; ++i)
    {
        observationDates.push_back(observationDate);
        observationDate += 1;
    }

    static const double arr[] = {0.000908, 0.000895, 0.000899, 0.000899, 0.000899, 0.000905, 0.000902,
                                 0.000900, 0.000904, 0.000905, 0.000905, 0.000905, 0.000905, 0.000910,
                                 0.000920, 0.000913, 0.000910, 0.000910, 0.000910, 0.000915, 0.000910,
                                 0.000915, 0.000905, 0.000905, 0.000905, 0.000905, 0.000900, 0.000905,
                                 0.000915, 0.000911, 0.00088};
    vector<double> rates(arr, arr + sizeof(arr) / sizeof(arr[0]));

    Date startDate = Date(01, Oct, 2014);
    Date endDate = Date(31, Oct, 2014);

    HistoricFedFundsCurve fed = HistoricFedFundsCurve(observationDates, rates);

    BOOST_REQUIRE(fed.isOK());

    vector<Date> marginCallDates;
    marginCallDates.push_back(Date(06, Oct, 2014));
    marginCallDates.push_back(Date(10, Oct, 2014));
    marginCallDates.push_back(Date(14, Oct, 2014));
    marginCallDates.push_back(Date(16, Oct, 2014));
    marginCallDates.push_back(Date(17, Oct, 2014));
    marginCallDates.push_back(Date(21, Oct, 2014));
    marginCallDates.push_back(Date(31, Oct, 2014));
    static const double arr2[] = {620000, 880000, 730000, 1380000, 1430000, -680000, -640000};
    vector<double> marginCallAmounts(arr2, arr2 + sizeof(arr2) / sizeof(arr2[0]));

    double marginInterest =
        fed.calculateNonCompoundedInterestOnMargin(0, startDate, endDate, marginCallDates, marginCallAmounts);
    BOOST_REQUIRE(abs(marginInterest - 211.83) < 1e-10);
}

test_suite* FixingRateTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("Fixing Rate source");

    suite->add(BOOST_TEST_CASE(&FixingRateTest::testDailyFixingCurve));
    suite->add(BOOST_TEST_CASE(&FixingRateTest::testMonthlyFixingCurve));
    suite->add(BOOST_TEST_CASE(&FixingRateTest::testHistoricFedFundsCurve));

    return suite;
}
