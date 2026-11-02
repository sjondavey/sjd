#include "sjdTest.h"

using namespace boost::unit_test_framework;

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace
{

    boost::timer t;

    void startTimer()
    {
        t.restart();
    }
    void stopTimer()
    {
        double seconds = t.elapsed();
        int hours = int(seconds / 3600);
        seconds -= hours * 3600;
        int minutes = int(seconds / 60);
        seconds -= minutes * 60;
        std::cout << " \nTests completed in ";
        if (hours > 0)
            std::cout << hours << " h ";
        if (hours > 0 || minutes > 0)
            std::cout << minutes << " m ";
        std::cout << std::fixed << std::setprecision(0) << seconds << " s\n" << std::endl;
    }

    void configure()
    {
        /* if needed, either or both the lines below can be
           uncommented and/or changed to run the test suite with a
           different configuration. In the future, we'll need a
           mechanism that doesn't force us to recompile (possibly a
           couple of command-line flags for the test suite?)
        */

        // QuantLib::Settings::instance().includeReferenceDateCashFlows() = true;
        // QuantLib::Settings::instance().includeTodaysCashFlows() = boost::none;
    }

}

test_suite* init_unit_test_suite(int, char*[])
{

    std::string header = "Testing SJD Libraries";
    std::string rule = std::string(header.length(), '=');

    BOOST_TEST_MESSAGE(rule);
    BOOST_TEST_MESSAGE(header);
    BOOST_TEST_MESSAGE(rule);
    test_suite* test = BOOST_TEST_SUITE("sjd Library test suite");

    test->add(BOOST_TEST_CASE(startTimer));
    test->add(BOOST_TEST_CASE(configure));

    test->add(MathsFunctionsTest::suite());
    test->add(Maths2DInterpTest::suite());

    test->add(Black76Test::suite());
    test->add(AsianOptionTest::suite());

    test->add(InterestRateConventionTest::suite());
    test->add(DiscountCurvesTest::suite());
    test->add(ForwardCurveTest::suite());
    test->add(CalendarMonthAverageTest::suite());
    test->add(ForwardCurveFXTest::suite());
    test->add(FixingRateTest::suite());
    test->add(VolatilityCurveTest::suite());
    test->add(VolatilitySurfacesTest::suite());

    test->add(CashflowTest::suite());
    test->add(ForwardContactTest::suite());
    test->add(OptionContactTest::suite());
    test->add(AsianOptionContactTest::suite());
    test->add(StructureTest::suite());
    test->add(PortfolioTest::suite());
    test->add(PortfolioEvolverTest::suite());
    test->add(MarginTest::suite());
    test->add(PortfolioInsuranceTest::suite());
    test->add(CVATest::suite());

    test->add(GoldenValuesTest::suite());

    test->add(BOOST_TEST_CASE(stopTimer));

    return test;
}