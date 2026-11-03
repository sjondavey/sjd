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

    class Probe : public Validatable
    {
    public:
        int runs = 0;
        bool fail = false;
        bool warn = false;

    protected:
        std::string validationRoot() const override { return "probe"; }

        void runChecks() override
        {
            ++runs;
            if (fail)
            {
                recordIssue("PROBE_BAD", "bad", {});
            }
            if (warn)
            {
                recordWarning("PROBE_NOTE", "note", {"anchor_date"});
            }
        }
    };

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

void ErrorsTest::testReportHelpers()
{
    BOOST_TEST_MESSAGE("Testing the validation report helpers");

    Probe probe;
    BOOST_CHECK(probe.isValid());
    BOOST_CHECK(probe.isOK());
    BOOST_CHECK_EQUAL(probe.runs, 0);

    ValidationReport report = probe.validate();
    BOOST_CHECK(report.isOK());
    BOOST_CHECK(report.toString().empty());
    BOOST_CHECK_EQUAL(report.toJson(), "[]");
    BOOST_CHECK_EQUAL(probe.runs, 1);
    BOOST_CHECK(probe.isOK());
    BOOST_CHECK_EQUAL(probe.runs, 1);

    probe.fail = true;
    BOOST_CHECK(probe.isValid());
    report = probe.validate();
    BOOST_CHECK(!probe.isValid());
    BOOST_CHECK(!report.isOK());
    BOOST_CHECK_EQUAL(probe.runs, 2);
    BOOST_CHECK(report.toString() == "probe: PROBE_BAD: bad");
    BOOST_CHECK(report.toJson().find("\"severity\":\"ERROR\"") != string::npos);
    BOOST_CHECK(report.errors().getIssues().size() == 1);
    BOOST_CHECK(report.warnings().getIssues().empty());
    BOOST_CHECK(!probe.isOK());
    BOOST_CHECK_EQUAL(probe.runs, 2);

    ValidationIssue warning("PROBE_NOTE", "note", ValidationSeverity::WARNING, {"probe", "anchor_date"});
    ValidationReport merged = report.merge(ValidationReport({warning}));
    BOOST_CHECK(!merged.isOK());
    BOOST_CHECK(merged.warnings().getIssues().size() == 1);
    BOOST_CHECK(merged.toString().find("probe/anchor_date: PROBE_NOTE: note") != string::npos);

    probe.fail = false;
    probe.warn = true;
    report = probe.validate();
    BOOST_CHECK(report.isOK());
    BOOST_CHECK(report.errors().getIssues().empty());
    BOOST_CHECK(report.warnings().getIssues().size() == 1);
    BOOST_CHECK(probe.getErrorMessages().empty());
    BOOST_CHECK(report.toString() == "probe/anchor_date: PROBE_NOTE: note");
}

test_suite* ErrorsTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("Errors");
    suite->add(BOOST_TEST_CASE(&ErrorsTest::testErrorCodes));
    suite->add(BOOST_TEST_CASE(&ErrorsTest::testValidationErrorCarriesTheReport));
    suite->add(BOOST_TEST_CASE(&ErrorsTest::testRequireAndAssert));
    suite->add(BOOST_TEST_CASE(&ErrorsTest::testConfigurationSites));
    suite->add(BOOST_TEST_CASE(&ErrorsTest::testReportHelpers));
    return suite;
}
