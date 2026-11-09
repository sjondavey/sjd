#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

BOOST_AUTO_TEST_SUITE(VolatilityCurveTest)

BOOST_AUTO_TEST_CASE(testVolatilityCurveConstruction)
{
    BOOST_TEST_MESSAGE("Testing VolatilityCurve Construction ...");
    Calendar calendar = SouthAfrica();
    Date anchorDate = Date(27, Jan, 2012);
    vector<Date> observationDates, incorrectObservationDates;
    vector<double> volatility;
    size_t i;
    for (i = 1; i <= 12; ++i)
    {
        Date date = calendar.advance(anchorDate, static_cast<int>(i), Months);
        observationDates.push_back(date);
        incorrectObservationDates.push_back(date);
        volatility.push_back(0.2);
    }
    incorrectObservationDates.push_back(calendar.advance(anchorDate, static_cast<int>(i + 1), Months));
    Actual365Fixed dayCounter;
    ArrayInterpolatorType type = ArrayInterpolatorType::LINEAR;
    bool allowExtrapolation = false;

    VolatilityCurve vc(anchorDate, incorrectObservationDates, volatility, dayCounter, type,
                       allowExtrapolation);
    BOOST_CHECK(vc.validate().isOK() == false);

    VolatilityCurve* vc2 =
        new VolatilityCurve(anchorDate, observationDates, volatility, dayCounter, type, allowExtrapolation);
    BOOST_CHECK(vc2->validate().isOK() == true);
    Date d1 = calendar.advance(anchorDate, 5, Months);
    Date d2 = calendar.advance(anchorDate, 13, Months);
    Date d3 = calendar.advance(anchorDate, -1, Months);
    BOOST_CHECK(vc2->isInRange(d1) == true);
    BOOST_CHECK(vc2->isInRange(d2) == false);
    BOOST_CHECK(vc2->isInRange(d3) == false);
    // Check the behaviour on the anchor date
    BOOST_CHECK_SMALL(vc2->getVolatility(anchorDate) - vc2->getVolatility(observationDates.front()),
                      EPSILON_FOR_DOUBLES);

    allowExtrapolation = !allowExtrapolation;
    delete vc2;
    vc2 = new VolatilityCurve(anchorDate, observationDates, volatility, dayCounter, type, allowExtrapolation);
    BOOST_CHECK(vc2->isInRange(d1) == true);
    BOOST_CHECK(vc2->isInRange(d2) == true);
    BOOST_CHECK(vc2->isInRange(d3) == false);
    // Check the behaviour on the anchor date
    BOOST_CHECK_SMALL(vc2->getVolatility(anchorDate) - vc2->getVolatility(observationDates.front()),
                      EPSILON_FOR_DOUBLES);

    delete vc2;
}
BOOST_AUTO_TEST_CASE(testVolatilityCurveConsistencyBetweenVolAndSD)
{
    BOOST_TEST_MESSAGE("Testing Volatiltiy Curve for consistency between getVol and getSD ...");
    GenerateRates re = GenerateRates();
    Calendar defaultCalendar = re.defaultCalendar;
    Date anchorDate = re.defaultAnchorDate;
    DayCounter dayCounter = re.defaultDayCounter;
    std::shared_ptr<VolatilityCurve> vc = re.getDefaultVolatiltiyCurve(anchorDate);

    Date fromDate = defaultCalendar.advance(anchorDate, 1, Months);
    double fromYF = dayCounter.yearFraction(anchorDate, fromDate);
    double fromSD = vc->getStandardDeviation(fromDate);
    double fromVol = vc->getVolatility(fromDate);
    BOOST_CHECK_SMALL(fromVol - fromSD / sqrt(fromYF), EPSILON_FOR_DOUBLES);

    Date toDate = defaultCalendar.advance(anchorDate, 9, Months);
    double toYF = dayCounter.yearFraction(anchorDate, toDate);
    double toSD = vc->getStandardDeviation(toDate);
    double toVol = vc->getVolatility(toDate);
    BOOST_CHECK_SMALL(toVol - toSD / sqrt(toYF), EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testVolatilityCurveValues)
{
    BOOST_TEST_MESSAGE("Testing Volatiltiy Curve values ...");
    GenerateRates re = GenerateRates();
    Calendar calendar = re.defaultCalendar;
    Date anchorDate = re.defaultAnchorDate;
    DayCounter dayCounter = re.defaultDayCounter;
    std::shared_ptr<VolatilityCurve> vc = re.getDefaultVolatiltiyCurve(anchorDate);

    Date pillar = calendar.advance(anchorDate, 1, Years);
    BOOST_CHECK_SMALL(vc->getVolatility(pillar) - re.defaultVolatiltiy, EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(vc->getVolatility(anchorDate) - re.defaultVolatiltiy, EPSILON_FOR_DOUBLES);

    // A flat curve stays flat between the anchor and the first pillar, because the
    // curve interpolates total variance rather than standard deviation.
    Date date1 = Date(13, Mar, 2012);
    double time1 = dayCounter.yearFraction(anchorDate, date1);
    double sd = vc->getStandardDeviation(date1);
    double vol = vc->getVolatility(date1);
    BOOST_CHECK_SMALL(vol - sd / sqrt(time1), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(vol - re.defaultVolatiltiy, EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testVolatilityCurveRoll)
{
    BOOST_TEST_MESSAGE("Testing Volatiltiy Curve roll forward ...");

    GenerateRates re = GenerateRates();
    Calendar defaultCalendar = re.defaultCalendar;
    Date anchorDate = re.defaultAnchorDate;
    DayCounter dayCounter = re.defaultDayCounter;
    std::shared_ptr<VolatilityCurve> vc = re.getDefaultVolatiltiyCurve(anchorDate);

    Date rollDate = defaultCalendar.advance(anchorDate, 1, Years);
    Date checkDate = defaultCalendar.advance(anchorDate, 2, Years);

    std::shared_ptr<VolatilityRateSource> rolledCurve = vc->rollForward(rollDate);

    BOOST_REQUIRE(rolledCurve->validate().isOK());

    double v1 = vc->getStandardDeviation(rollDate);
    double v2 = vc->getStandardDeviation(checkDate);
    double yf = (dayCounter.yearFraction(rollDate, checkDate));
    double v12 = sqrt((pow(v2, 2) - pow(v1, 2)) / yf);
    double rolledVol = rolledCurve->getVolatility(checkDate);
    BOOST_CHECK_SMALL(v12 - rolledVol, EPSILON_FOR_DOUBLES);
}

BOOST_AUTO_TEST_SUITE_END()
