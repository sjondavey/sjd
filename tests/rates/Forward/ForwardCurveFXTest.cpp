#include "ForwardCurveFXTest.h"

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

void ForwardCurveFXTest::getDefaultDatesAndDF(Date anchorDate,
                                              std::shared_ptr<DiscountCurve>& domesticDRS,
                                              std::shared_ptr<DiscountCurve>& foreignDRS,
                                              std::shared_ptr<VolatilityCurve>& vrs)
{
    vector<Date> domesticObservationDates, foreignObservationDates;
    vector<double> domesticDF, foreignDF;
    Calendar calendar = NullCalendar();
    Date date;
    size_t i;
    for (i = 0; i <= 12; ++i)
    {
        date = calendar.advance(anchorDate, static_cast<int>(i), Months);
        domesticObservationDates.push_back(date);
        foreignObservationDates.push_back(date);
        double daysAhead = date - anchorDate;
        domesticDF.push_back(exp(-0.06 * daysAhead / 365.0));
        foreignDF.push_back(exp(-0.03 * daysAhead / 360.0));
    }
    ++i;
    date = calendar.advance(anchorDate, static_cast<int>(i), Months);
    foreignObservationDates.push_back(date);
    double daysAhead = date - anchorDate;
    foreignDF.push_back(exp(-0.03 * daysAhead / 360.0));

    domesticDRS = std::make_shared<DiscountCurve>(anchorDate, domesticObservationDates, domesticDF,
                                                  ArrayInterpolatorType::LINEAR, false);
    foreignDRS = std::make_shared<DiscountCurve>(anchorDate, foreignObservationDates, foreignDF,
                                                 ArrayInterpolatorType::LINEAR, false);

    vector<double> volatility = vector<double>(domesticObservationDates.size(), 0.2);
    std::shared_ptr<DayCounter> dayCounter = std::make_shared<Actual365Fixed>();
    vrs = std::make_shared<VolatilityCurve>(anchorDate, domesticObservationDates, volatility, dayCounter,
                                            ArrayInterpolatorType::LINEAR, false);
}

void ForwardCurveFXTest::testForwardCurveConstruction()
{
    BOOST_TEST_MESSAGE("Testing Construction of ForwardCurveFX, ForwardCurveStochasticFX and "
                       "ForwardCurveDeterministicFX ...");
    Date anchorDate = Date(27, Jan, 2012);
    Calendar calendar = NullCalendar();
    std::shared_ptr<DiscountCurve> domesticDRS;
    std::shared_ptr<DiscountCurve> foreignDRS;
    std::shared_ptr<VolatilityCurve> vrs;
    getDefaultDatesAndDF(anchorDate, domesticDRS, foreignDRS, vrs);

    double spot = 1100.0;
    Date spotDate = calendar.advance(anchorDate, 2, Days);
    std::shared_ptr<ForwardCurveFX> fxCurve =
        std::make_shared<ForwardCurveFX>(anchorDate, spot, spotDate, domesticDRS, foreignDRS);
    BOOST_REQUIRE(fxCurve->isOK());
    double manualSpotAtT0 =
        spot * domesticDRS->getDiscountFactor(spotDate) / foreignDRS->getDiscountFactor(spotDate);
    BOOST_CHECK(abs(fxCurve->getSpotAtT0() - manualSpotAtT0) < EPSILON_FOR_DOUBLES);
    Date fwdDate = calendar.advance(anchorDate, 173, Days);
    double manualFwd =
        manualSpotAtT0 * foreignDRS->getDiscountFactor(fwdDate) / domesticDRS->getDiscountFactor(fwdDate);
    BOOST_CHECK(abs(fxCurve->getForward(fwdDate) - manualFwd) < 10 * EPSILON_FOR_DOUBLES);

    std::shared_ptr<ForwardCurveStochasticFX> stochasticFXCurve =
        std::make_shared<ForwardCurveStochasticFX>(anchorDate, spot, spotDate, domesticDRS, foreignDRS, vrs);
    BOOST_REQUIRE(stochasticFXCurve->isOK());
    BOOST_CHECK(abs(fxCurve->getSpotAtT0() - stochasticFXCurve->getSpotAtT0()) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(stochasticFXCurve->getForward(fwdDate) - manualFwd) < EPSILON_FOR_DOUBLES);

    std::shared_ptr<ForwardCurveDeterministicFX> fxCurveDeterminstic =
        std::make_shared<ForwardCurveDeterministicFX>(anchorDate, spot, spotDate, domesticDRS, foreignDRS);
    BOOST_REQUIRE(fxCurveDeterminstic->isOK());
    BOOST_CHECK(abs(fxCurve->getSpotAtT0() - fxCurveDeterminstic->getSpotAtT0()) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(fxCurveDeterminstic->getForward(fwdDate) - manualFwd) < EPSILON_FOR_DOUBLES);
}

void ForwardCurveFXTest::testDeterministicCurve()
{
    BOOST_TEST_MESSAGE("Testing rolling and bumping of ForwardCurveDeterministicFX ...");
    Date anchorDate = Date(27, Jan, 2012);
    Calendar calendar = NullCalendar();
    std::shared_ptr<DiscountCurve> domesticDRS;
    std::shared_ptr<DiscountCurve> foreignDRS;
    std::shared_ptr<VolatilityCurve> vrs;
    getDefaultDatesAndDF(anchorDate, domesticDRS, foreignDRS, vrs);

    double spot = 1100.0;
    Date spotDate = calendar.advance(anchorDate, 2, Days);
    std::shared_ptr<ForwardCurveDeterministicFX> fxCurveDeterminstic =
        std::make_shared<ForwardCurveDeterministicFX>(anchorDate, spot, domesticDRS, foreignDRS);

    double bump = 1.0;
    std::shared_ptr<ForwardRateSource> bumpedCurve = fxCurveDeterminstic->parallelBump(bump);
    BOOST_REQUIRE(bumpedCurve->isOK());
    BOOST_CHECK(abs(bumpedCurve->getForward(anchorDate) - (fxCurveDeterminstic->getSpotAtT0() + bump)) <
                EPSILON_FOR_DOUBLES * 100);

    Date fwdDate = calendar.advance(anchorDate, 173, Days);
    double manualFwd = fxCurveDeterminstic->getForward(fwdDate);
    std::shared_ptr<ForwardRateSource> rolledCurve = fxCurveDeterminstic->rollForward(fwdDate);
    BOOST_REQUIRE(rolledCurve->isOK());
    BOOST_CHECK(abs(rolledCurve->getForward(fwdDate) - manualFwd) < EPSILON_FOR_DOUBLES * 100);
}

void ForwardCurveFXTest::testStochasticCurve()
{
    BOOST_TEST_MESSAGE("Testing rolling and bumping of ForwardCurveStochasticFX ...");
    Date anchorDate = Date(27, Jan, 2012);
    Calendar calendar = NullCalendar();
    std::shared_ptr<DiscountCurve> domesticDRS;
    std::shared_ptr<DiscountCurve> foreignDRS;
    std::shared_ptr<VolatilityCurve> vrs;
    getDefaultDatesAndDF(anchorDate, domesticDRS, foreignDRS, vrs);

    double spot = 1100.0;
    Date spotDate = calendar.advance(anchorDate, 2, Days);
    std::shared_ptr<ForwardCurveStochasticFX> stochasticFXCurve =
        std::make_shared<ForwardCurveStochasticFX>(anchorDate, spot, spotDate, domesticDRS, foreignDRS, vrs);

    Date fwdDate = calendar.advance(anchorDate, 173, Days);
    double normalRV = 1.64;
    std::shared_ptr<ForwardRateSource> rolledCurve = stochasticFXCurve->rollForward(fwdDate, normalRV);
    BOOST_REQUIRE(rolledCurve->isOK());
    double manualFwd = stochasticFXCurve->getForward(fwdDate);
    double sd = vrs->getStandardDeviation(fwdDate);
    manualFwd *= exp(-sd * sd / 2.0 + normalRV * sd);
    BOOST_CHECK(abs(rolledCurve->getForward(fwdDate) - manualFwd) < EPSILON_FOR_DOUBLES * 100);
}

test_suite* ForwardCurveFXTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("FX Forward Curves");

    suite->add(BOOST_TEST_CASE(&ForwardCurveFXTest::testForwardCurveConstruction));
    suite->add(BOOST_TEST_CASE(&ForwardCurveFXTest::testDeterministicCurve));
    suite->add(BOOST_TEST_CASE(&ForwardCurveFXTest::testStochasticCurve));

    return suite;
}
