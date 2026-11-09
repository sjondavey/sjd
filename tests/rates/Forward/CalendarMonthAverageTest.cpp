#include <boost/test/unit_test.hpp>
#include <iostream>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <sjd/rates/Forward/Deterministic/ForwardCurveCMADeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveCMAStochasticFwdVol.h>
#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

BOOST_AUTO_TEST_SUITE(CalendarMonthAverageTest)

BOOST_AUTO_TEST_CASE(testConstruction)
{
    BOOST_TEST_MESSAGE("Testing Calendar Month Average Futures Curve Construction ...");

    Calendar calendar = NullCalendar();
    Date anchorDate = Date(27, Jan, 2012);
    vector<Date> observationDates, incorrectObservationDates;
    vector<double> futures;
    size_t i;
    bool allowExtrapolation = false;
    // Case 1 NULL inputs
    std::shared_ptr<ForwardCurveCMADeterministic> fc = std::make_shared<ForwardCurveCMADeterministic>(
        anchorDate, incorrectObservationDates, futures, sjd::ArrayInterpolatorType::LINEAR,
        allowExtrapolation);
    BOOST_CHECK(fc->validate().isOK() == false);

    GenerateRates gr = GenerateRates();
    std::shared_ptr<VolatilitySurfaceMoneynessCMA> vc =
        gr.getDefaultCalendarMonthAverageMoneynessSurface(anchorDate);
    std::shared_ptr<ForwardCurveCMAStochasticFwdVol> sfc = std::make_shared<ForwardCurveCMAStochasticFwdVol>(
        anchorDate, incorrectObservationDates, futures, sjd::ArrayInterpolatorType::LINEAR,
        allowExtrapolation, vc);
    BOOST_CHECK(sfc->validate().isOK() == false);

    for (i = 1; i <= 12; ++i)
    {
        Date date = calendar.advance(anchorDate, static_cast<int>(i), Months);
        observationDates.push_back(date);
        incorrectObservationDates.push_back(date);
        futures.push_back(110 - (double)i / 10.0);
    }
    incorrectObservationDates.push_back(calendar.advance(anchorDate, static_cast<int>(i + 1), Months));
    fc = std::make_shared<ForwardCurveCMADeterministic>(anchorDate, incorrectObservationDates, futures,
                                                        sjd::ArrayInterpolatorType::LINEAR,
                                                        allowExtrapolation);
    BOOST_CHECK(fc->validate().isOK() == false);
    sfc = std::make_shared<ForwardCurveCMAStochasticFwdVol>(anchorDate, incorrectObservationDates, futures,
                                                            sjd::ArrayInterpolatorType::LINEAR,
                                                            allowExtrapolation, vc);
    BOOST_CHECK(sfc->validate().isOK() == false);

    std::shared_ptr<ForwardCurveCMADeterministic> fc2 = std::make_shared<ForwardCurveCMADeterministic>(
        anchorDate, observationDates, futures, sjd::ArrayInterpolatorType::LINEAR, allowExtrapolation);
    BOOST_CHECK(fc2->validate().isOK() == true);
    std::shared_ptr<ForwardCurveCMAStochasticFwdVol> sfc2 = std::make_shared<ForwardCurveCMAStochasticFwdVol>(
        anchorDate, observationDates, futures, sjd::ArrayInterpolatorType::LINEAR, allowExtrapolation, vc);
    BOOST_CHECK(sfc2->validate().isOK() == true);

    Date d1 = calendar.advance(anchorDate, 5, Months);
    Date d2 = calendar.advance(anchorDate, 13, Months);
    Date d3 = calendar.advance(anchorDate, -1, Months);
    BOOST_CHECK(fc2->isInRange(d1) == true);
    BOOST_CHECK(fc2->isInRange(d2) == false);
    BOOST_CHECK(fc2->isInRange(d3) == false);

    BOOST_CHECK(sfc2->isInRange(d1) == true);
    BOOST_CHECK(sfc2->isInRange(d2) == false);
    BOOST_CHECK(sfc2->isInRange(d3) == false);

    allowExtrapolation = !allowExtrapolation;
    fc2 = std::make_shared<ForwardCurveCMADeterministic>(
        anchorDate, observationDates, futures, sjd::ArrayInterpolatorType::LINEAR, allowExtrapolation);
    BOOST_CHECK(fc2->isInRange(d1) == true);
    BOOST_CHECK(fc2->isInRange(d2) == true);
    BOOST_CHECK(fc2->isInRange(d3) == false);

    sfc2 = std::make_shared<ForwardCurveCMAStochasticFwdVol>(
        anchorDate, observationDates, futures, sjd::ArrayInterpolatorType::LINEAR, allowExtrapolation, vc);
    BOOST_CHECK(sfc2->isInRange(d1) == true);
    BOOST_CHECK(sfc2->isInRange(d2) == true);
    BOOST_CHECK(sfc2->isInRange(d3) == false);
}
BOOST_AUTO_TEST_CASE(testValues)
{
    BOOST_TEST_MESSAGE("Testing Calendar Month Average Forward Curve Valuations ...");

    Calendar calendar = NullCalendar();
    Date anchorDate = Date(27, Jan, 2012);
    vector<Date> observationDates;
    vector<double> futures;
    size_t i;
    bool allowExtrapolation = false;
    for (i = 1; i <= 6; ++i)
    {
        Date date = calendar.advance(anchorDate, static_cast<int>(2 * i), Months);
        observationDates.push_back(date);
        futures.push_back(110 - ((double)i * 2) / 10.0);
    }

    ForwardCurveCMADeterministic* fc = new ForwardCurveCMADeterministic(
        anchorDate, observationDates, futures, sjd::ArrayInterpolatorType::LINEAR, allowExtrapolation);
    BOOST_REQUIRE(fc->validate().isOK() == true);
    BOOST_CHECK_SMALL(fc->getForward(anchorDate) - futures[0], EPSILON_FOR_DOUBLES);

    GenerateRates gr = GenerateRates();
    std::shared_ptr<VolatilitySurfaceMoneynessCMA> vc =
        gr.getDefaultCalendarMonthAverageMoneynessSurface(anchorDate);
    std::shared_ptr<ForwardCurveCMAStochasticFwdVol> sfc = std::make_shared<ForwardCurveCMAStochasticFwdVol>(
        anchorDate, observationDates, futures, sjd::ArrayInterpolatorType::LINEAR, allowExtrapolation, vc);
    BOOST_REQUIRE(sfc->validate().isOK() == true);
    BOOST_CHECK_SMALL(sfc->getForward(anchorDate) - futures[0], EPSILON_FOR_DOUBLES);

    Date d1 = calendar.advance(anchorDate, 3, Months);
    double interpolatedRate = (futures[0] + futures[1]) / 2.0;
    BOOST_CHECK_SMALL(fc->getForward(d1) - interpolatedRate, EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(sfc->getForward(d1) - interpolatedRate, EPSILON_FOR_DOUBLES);

    d1 = calendar.advance(d1, -3, Days); // different date in the same month
    BOOST_CHECK_SMALL(fc->getForward(d1) - interpolatedRate, EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(sfc->getForward(d1) - interpolatedRate, EPSILON_FOR_DOUBLES);

    delete fc;
}
BOOST_AUTO_TEST_CASE(testRoll_CMA)
{
    BOOST_TEST_MESSAGE("Testing Calendar Month Average Forward Curve roll ...");

    GenerateRates re = GenerateRates();

    Calendar calendar = re.defaultCalendar;
    Date anchorDate = re.defaultAnchorDate;
    vector<Date> observationDates = re.defaultCurveDates;
    vector<double> futures;
    size_t i;
    for (i = 1; i <= observationDates.size(); ++i)
    {
        futures.push_back(110 - (double)i / 10.0);
    }
    bool allowExtrapolation = false;
    std::shared_ptr<VolatilitySurfaceMoneynessCMA> vrs =
        re.getDefaultCalendarMonthAverageMoneynessSurface(anchorDate);
    std::shared_ptr<ForwardCurveCMAStochasticFwdVol> fc = std::make_shared<ForwardCurveCMAStochasticFwdVol>(
        anchorDate, observationDates, futures, sjd::ArrayInterpolatorType::LINEAR, allowExtrapolation, vrs);

    double rn = 0.12354;

    std::shared_ptr<ForwardRateSource> rolledFwd = fc->rollForward(observationDates[1], rn);
    BOOST_CHECK(rolledFwd->validate().isOK());

    double sqrtDt = sqrt((observationDates[1] - anchorDate) / 365.0);
    // std::shared_ptr<ForwardRateSource> bumpedCurve = rolledFwd->bumpStochastically(sqrtDt, vrs, rn);
    Date dateToCheck = observationDates[3]; // calendar.advance(anchorDate, 39, Months);

    double manualRolledForward = fc->getForward(dateToCheck);
    double vol = vrs->getVolatility(dateToCheck);
    double sd = vol * sqrtDt;
    manualRolledForward *= exp(-sd * sd / 2 + sd * rn);
    BOOST_CHECK_SMALL(manualRolledForward - (rolledFwd->getForward(dateToCheck)), 10 * EPSILON_FOR_DOUBLES);
}

BOOST_AUTO_TEST_SUITE_END()
