#include "VolatilityCurveTest.h"

using namespace std;
using namespace QuantLib;
using namespace boost;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

void VolatilityCurveTest::testVolatilityCurveConstruction()
{
    BOOST_TEST_MESSAGE("Testing VolatilityCurve Construction ...");
    Calendar calendar = SouthAfrica();
    Date anchorDate = Date(27, Jan, 2012);
    vector<Date> observationDates, incorrectObservationDates;
    vector<double> volatility;
    size_t i;
    for (i = 1; i <= 12; ++i)
    {
        Date date = calendar.advance(anchorDate, i, Months);
        observationDates.push_back(date);
        incorrectObservationDates.push_back(date);
        volatility.push_back(0.2);
    }
    incorrectObservationDates.push_back(calendar.advance(anchorDate, i + 1, Months));
    std::shared_ptr<DayCounter> dayCounter = std::shared_ptr<DayCounter>(new Actual365Fixed());
    ArrayInterpolatorType type = LINEAR;
    bool allowExtrapolation = false;

    VolatilityCurve vc(anchorDate, incorrectObservationDates, volatility, dayCounter, type,
                       allowExtrapolation);
    BOOST_CHECK(vc.isOK() == false);

    VolatilityCurve* vc2 =
        new VolatilityCurve(anchorDate, observationDates, volatility, dayCounter, type, allowExtrapolation);
    BOOST_CHECK(vc2->isOK() == true);
    Date d1 = calendar.advance(anchorDate, 5, Months);
    Date d2 = calendar.advance(anchorDate, 13, Months);
    Date d3 = calendar.advance(anchorDate, -1, Months);
    BOOST_CHECK(vc2->isInRange(d1) == true);
    BOOST_CHECK(vc2->isInRange(d2) == false);
    BOOST_CHECK(vc2->isInRange(d3) == false);
    // Check the behaviour on the anchor date
    BOOST_CHECK(abs(vc2->getVolatility(anchorDate) - vc2->getVolatility(observationDates.front())) <
                EPSILON_FOR_DOUBLES);

    allowExtrapolation = !allowExtrapolation;
    delete vc2;
    vc2 = new VolatilityCurve(anchorDate, observationDates, volatility, dayCounter, type, allowExtrapolation);
    BOOST_CHECK(vc2->isInRange(d1) == true);
    BOOST_CHECK(vc2->isInRange(d2) == true);
    BOOST_CHECK(vc2->isInRange(d3) == false);
    // Check the behaviour on the anchor date
    BOOST_CHECK(abs(vc2->getVolatility(anchorDate) - vc2->getVolatility(observationDates.front())) <
                EPSILON_FOR_DOUBLES);

    delete vc2;
}

void VolatilityCurveTest::testVolatilityCurveConsistencyBetweenVolAndSD()
{
    BOOST_TEST_MESSAGE("Testing Volatiltiy Curve for consistency between getVol and getSD ...");
    GenerateRates re = GenerateRates();
    std::shared_ptr<Calendar> defaultCalendar = re.defaultCalendar;
    Date anchorDate = re.defaultAnchorDate;
    std::shared_ptr<DayCounter> dayCounter = re.defaultDayCounter;
    std::shared_ptr<VolatilityCurve> vc = re.getDefaultVolatiltiyCurve(anchorDate);

    Date fromDate = defaultCalendar->advance(anchorDate, 1, Months);
    double fromYF = dayCounter->yearFraction(anchorDate, fromDate);
    double fromSD = vc->getStandardDeviation(fromDate);
    double fromVol = vc->getVolatility(fromDate);
    BOOST_CHECK(abs(fromVol - fromSD / sqrt(fromYF)) < EPSILON_FOR_DOUBLES);

    Date toDate = defaultCalendar->advance(anchorDate, 9, Months);
    double toYF = dayCounter->yearFraction(anchorDate, toDate);
    double toSD = vc->getStandardDeviation(toDate);
    double toVol = vc->getVolatility(toDate);
    BOOST_CHECK(abs(toVol - toSD / sqrt(toYF)) < EPSILON_FOR_DOUBLES);
}

void VolatilityCurveTest::testVolatilityCurveValues()
{
    BOOST_TEST_MESSAGE("Testing Volatiltiy Curve values ...");
    GenerateRates re = GenerateRates();
    std::shared_ptr<Calendar> calendar = re.defaultCalendar;
    Date anchorDate = re.defaultAnchorDate;
    std::shared_ptr<DayCounter> dayCounter = re.defaultDayCounter;
    std::shared_ptr<VolatilityCurve> vc = re.getDefaultVolatiltiyCurve(anchorDate);

    Date pillar = calendar->advance(anchorDate, 1, Years);
    BOOST_CHECK(abs(vc->getVolatility(pillar) - re.defaultVolatiltiy) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(vc->getVolatility(anchorDate) - re.defaultVolatiltiy) < EPSILON_FOR_DOUBLES);

    // 13 Mar 2012 sits between the anchor and the first yearly pillar. Standard
    // deviation is interpolated from zero at the anchor, so the volatility there
    // is below the flat input volatility.
    Date date1 = Date(13, Mar, 2012);
    double time1 = dayCounter->yearFraction(anchorDate, date1);
    double sd = vc->getStandardDeviation(date1);
    double vol = vc->getVolatility(date1);
    BOOST_CHECK(abs(vol - sd / sqrt(time1)) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(vol < re.defaultVolatiltiy);
}

void VolatilityCurveTest::testVolatilityCurveRoll()
{
    BOOST_TEST_MESSAGE("Testing Volatiltiy Curve roll forward ...");

    GenerateRates re = GenerateRates();
    std::shared_ptr<Calendar> defaultCalendar = re.defaultCalendar;
    Date anchorDate = re.defaultAnchorDate;
    std::shared_ptr<DayCounter> dayCounter = re.defaultDayCounter;
    std::shared_ptr<VolatilityCurve> vc = re.getDefaultVolatiltiyCurve(anchorDate);

    Date rollDate = defaultCalendar->advance(anchorDate, 1, Years);
    Date checkDate = defaultCalendar->advance(anchorDate, 2, Years);

    std::shared_ptr<VolatilityRateSource> rolledCurve = vc->rollForward(rollDate);

    BOOST_REQUIRE(rolledCurve->isOK());

    double v1 = vc->getStandardDeviation(rollDate);
    double v2 = vc->getStandardDeviation(checkDate);
    double yf = (dayCounter->yearFraction(rollDate, checkDate));
    double v12 = sqrt((pow(v2, 2) - pow(v1, 2)) / yf);
    double rolledVol = rolledCurve->getVolatility(checkDate);
    BOOST_CHECK(abs(v12 - rolledVol) < EPSILON_FOR_DOUBLES);
}

test_suite* VolatilityCurveTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("Volatility Rate Sources");
    suite->add(BOOST_TEST_CASE(&VolatilityCurveTest::testVolatilityCurveConstruction));
    suite->add(BOOST_TEST_CASE(&VolatilityCurveTest::testVolatilityCurveConsistencyBetweenVolAndSD));
    suite->add(BOOST_TEST_CASE(&VolatilityCurveTest::testVolatilityCurveValues));
    suite->add(BOOST_TEST_CASE(&VolatilityCurveTest::testVolatilityCurveRoll));

    return suite;
}
