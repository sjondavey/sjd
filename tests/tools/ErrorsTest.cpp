#include "ErrorsTest.h"

#include <sjd/rates/Forward/ForwardCurveFactory.h>
#include <sjd/tools/Calendars.h>
#include <sjd/tools/Errors.h>

using namespace std;
using namespace QuantLib;
using namespace boost::unit_test_framework;
using namespace sjd;

namespace
{

    void requireFalse()
    {
        SJD_REQUIRE(false);
    }

    void assertFalse()
    {
        SJD_ASSERT(false);
    }

}

void ErrorsTest::testErrorCodes()
{
    BOOST_TEST_MESSAGE("Testing error codes");

    sjd::Error base("SAMPLE", "base message");
    BOOST_CHECK_EQUAL(base.code(), "SAMPLE");
    BOOST_CHECK_EQUAL(string(base.what()), "base message");

    ConfigurationError configuration("unknown name");
    BOOST_CHECK_EQUAL(configuration.code(), "CONFIGURATION");
    BOOST_CHECK_EQUAL(string(configuration.what()), "unknown name");

    NumericalError numerical("solver failed");
    BOOST_CHECK_EQUAL(numerical.code(), "NUMERICAL");
    BOOST_CHECK_EQUAL(string(numerical.what()), "solver failed");

    auto throwConfiguration = [&]() { throw configuration; };
    auto throwNumerical = [&]() { throw numerical; };
    BOOST_CHECK_THROW(throwConfiguration(), sjd::Error);
    BOOST_CHECK_THROW(throwNumerical(), runtime_error);
}

void ErrorsTest::testValidationErrorCarriesTheReport()
{
    BOOST_TEST_MESSAGE("Testing ValidationError");

    ValidationIssue issue("CURVE_EMPTY", "the curve has no pillars", ValidationSeverity::ERROR, {"curve"});
    ValidationReport report(vector<ValidationIssue>{issue});
    ValidationError error("refusing to price an invalid curve", report);

    BOOST_CHECK_EQUAL(error.code(), "VALIDATION");
    BOOST_CHECK_EQUAL(string(error.what()), "refusing to price an invalid curve");
    BOOST_REQUIRE_EQUAL(error.report().getIssues().size(), 1u);
    BOOST_CHECK_EQUAL(error.report().getIssues().front().code, "CURVE_EMPTY");
    BOOST_CHECK(!error.report().isOK());
}

void ErrorsTest::testRequireAndAssert()
{
    BOOST_TEST_MESSAGE("Testing SJD_REQUIRE and SJD_ASSERT");

    SJD_REQUIRE(true);
    SJD_ASSERT(true);

    try
    {
        requireFalse();
        BOOST_ERROR("SJD_REQUIRE(false) did not throw");
    }
    catch (const sjd::Error& error)
    {
        BOOST_CHECK_EQUAL(error.code(), "REQUIRE");
    }

#ifndef NDEBUG
    try
    {
        assertFalse();
        BOOST_ERROR("SJD_ASSERT(false) did not throw");
    }
    catch (const sjd::Error& error)
    {
        BOOST_CHECK_EQUAL(error.code(), "ASSERT");
    }
#else
    assertFalse();
#endif
}

void ErrorsTest::testConfigurationSites()
{
    BOOST_TEST_MESSAGE("Testing configuration throws");

    BOOST_CHECK_THROW(calendarNamed("Narnia"), ConfigurationError);
    BOOST_CHECK_THROW(jointCalendar({}), ConfigurationError);

    try
    {
        calendarNamed("Narnia");
        BOOST_ERROR("calendarNamed did not throw");
    }
    catch (const ConfigurationError& error)
    {
        BOOST_CHECK(string(error.what()).find("Narnia") != string::npos);
    }

    Date anchor(1, January, 2026);
    BOOST_CHECK_THROW(makeForwardCurve(anchor, {}, {}, ArrayInterpolatorType::LINEAR, false,
                                       ForwardAveraging::CalendarMonth, ForwardRoll::SpotVolatility, {}),
                      ConfigurationError);
}

test_suite* ErrorsTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("Errors");
    suite->add(BOOST_TEST_CASE(&ErrorsTest::testErrorCodes));
    suite->add(BOOST_TEST_CASE(&ErrorsTest::testValidationErrorCarriesTheReport));
    suite->add(BOOST_TEST_CASE(&ErrorsTest::testRequireAndAssert));
    suite->add(BOOST_TEST_CASE(&ErrorsTest::testConfigurationSites));
    return suite;
}
