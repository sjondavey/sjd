#include <boost/test/unit_test.hpp>
#include <iostream>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <sjd/rates/Discount/InterestRateConvention.h>
#include <support/TestingParameters.h>

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

BOOST_AUTO_TEST_SUITE(InterestRateConventionTest)

BOOST_AUTO_TEST_CASE(testDFToRateConversion)
{
    BOOST_TEST_MESSAGE("Convert between rates in different convention and discount factors ...");

    Actual365Fixed dc;
    Date fromDate = Date(27, Jan, 2012);
    Date toDate = Date(16, Jul, 2014);
    double r = 0.085;

    std::shared_ptr<InterestRateConvention> convention =
        std::make_shared<InterestRateConvention>(dc, Compounded, Quarterly);
    double df = convention->rateToDF(r, fromDate, toDate);
    double manualDF = pow((1 + r / Quarterly), -Quarterly * dc.yearFraction(fromDate, toDate));
    BOOST_CHECK_SMALL(df - manualDF, EPSILON_FOR_DOUBLES);
    double r2 = convention->dfToRate(df, fromDate, toDate);
    BOOST_CHECK_SMALL(r - r2, EPSILON_FOR_DOUBLES);

    convention = convention = std::make_shared<InterestRateConvention>(dc, Continuous, Quarterly);
    df = convention->rateToDF(r, fromDate, toDate);
    manualDF = exp(-r * dc.yearFraction(fromDate, toDate));
    BOOST_CHECK_SMALL(df - manualDF, EPSILON_FOR_DOUBLES);
    r2 = convention->dfToRate(df, fromDate, toDate);
    BOOST_CHECK_SMALL(r - r2, EPSILON_FOR_DOUBLES);

    convention = convention = std::make_shared<InterestRateConvention>(dc, Simple, Quarterly);
    df = convention->rateToDF(r, fromDate, toDate);
    manualDF = 1.0 / (1 + r * dc.yearFraction(fromDate, toDate));
    BOOST_CHECK_SMALL(df - manualDF, EPSILON_FOR_DOUBLES);
    r2 = convention->dfToRate(df, fromDate, toDate);
    BOOST_CHECK_SMALL(r - r2, EPSILON_FOR_DOUBLES);
}

BOOST_AUTO_TEST_SUITE_END()
