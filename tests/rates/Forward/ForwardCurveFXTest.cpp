#include <boost/test/unit_test.hpp>
#include <iostream>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministicFX.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFX.h>
#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

BOOST_AUTO_TEST_SUITE(ForwardCurveFXTest)

void getDefaultDatesAndDF(Date anchorDate,
                          std::shared_ptr<DiscountCurve>& domesticDRS,
                          std::shared_ptr<DiscountCurve>& foreignDRS,
                          std::shared_ptr<VolatilityCurve>& vrs);

void getDefaultDatesAndDF(Date anchorDate,
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
    Actual365Fixed dayCounter;
    vrs = std::make_shared<VolatilityCurve>(anchorDate, domesticObservationDates, volatility, dayCounter,
                                            ArrayInterpolatorType::LINEAR, false);
}
BOOST_AUTO_TEST_CASE(testForwardCurveConstruction)
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
    BOOST_REQUIRE(fxCurve->validate().isOK());
    double manualSpotAtT0 =
        spot * domesticDRS->getDiscountFactor(spotDate) / foreignDRS->getDiscountFactor(spotDate);
    BOOST_CHECK_SMALL(fxCurve->getSpotAtT0() - manualSpotAtT0, EPSILON_FOR_DOUBLES);
    Date fwdDate = calendar.advance(anchorDate, 173, Days);
    double manualFwd =
        manualSpotAtT0 * foreignDRS->getDiscountFactor(fwdDate) / domesticDRS->getDiscountFactor(fwdDate);
    BOOST_CHECK_SMALL(fxCurve->getForward(fwdDate) - manualFwd, 10 * EPSILON_FOR_DOUBLES);

    std::shared_ptr<ForwardCurveStochasticFX> stochasticFXCurve =
        std::make_shared<ForwardCurveStochasticFX>(anchorDate, spot, spotDate, domesticDRS, foreignDRS, vrs);
    BOOST_REQUIRE(stochasticFXCurve->validate().isOK());
    BOOST_CHECK_SMALL(fxCurve->getSpotAtT0() - stochasticFXCurve->getSpotAtT0(), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(stochasticFXCurve->getForward(fwdDate) - manualFwd, EPSILON_FOR_DOUBLES);

    std::shared_ptr<ForwardCurveDeterministicFX> fxCurveDeterminstic =
        std::make_shared<ForwardCurveDeterministicFX>(anchorDate, spot, spotDate, domesticDRS, foreignDRS);
    BOOST_REQUIRE(fxCurveDeterminstic->validate().isOK());
    BOOST_CHECK_SMALL(fxCurve->getSpotAtT0() - fxCurveDeterminstic->getSpotAtT0(), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(fxCurveDeterminstic->getForward(fwdDate) - manualFwd, EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testDeterministicCurve)
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
    BOOST_REQUIRE(bumpedCurve->validate().isOK());
    BOOST_CHECK_SMALL(bumpedCurve->getForward(anchorDate) - (fxCurveDeterminstic->getSpotAtT0() + bump),
                      EPSILON_FOR_DOUBLES * 100);

    Date fwdDate = calendar.advance(anchorDate, 173, Days);
    double manualFwd = fxCurveDeterminstic->getForward(fwdDate);
    std::shared_ptr<ForwardRateSource> rolledCurve = fxCurveDeterminstic->rollForward(fwdDate);
    BOOST_REQUIRE(rolledCurve->validate().isOK());
    BOOST_CHECK_SMALL(rolledCurve->getForward(fwdDate) - manualFwd, EPSILON_FOR_DOUBLES * 100);
}
BOOST_AUTO_TEST_CASE(testStochasticCurve)
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
    BOOST_REQUIRE(rolledCurve->validate().isOK());
    double manualFwd = stochasticFXCurve->getForward(fwdDate);
    double sd = vrs->getStandardDeviation(fwdDate);
    manualFwd *= exp(-sd * sd / 2.0 + normalRV * sd);
    BOOST_CHECK_SMALL(rolledCurve->getForward(fwdDate) - manualFwd, EPSILON_FOR_DOUBLES * 100);
}

BOOST_AUTO_TEST_SUITE_END()
