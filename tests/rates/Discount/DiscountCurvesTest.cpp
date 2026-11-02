#include "DiscountCurvesTest.h"

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

void DiscountCurvesTest::testConstruction()
{
    BOOST_TEST_MESSAGE("Testing Discount Curve Construction ...");
    Calendar calendar = SouthAfrica();
    Date anchorDate = Date(27, Jan, 2012);
    vector<Date> observationDates, incorrectObservationDates;
    vector<double> df;
    std::shared_ptr<DayCounter> dayCounter = std::make_shared<Actual365Fixed>();
    bool allowExtrapolation = false;
    size_t i;
    for (i = 1; i <= 12; ++i)
    {
        Date date = calendar.advance(anchorDate, static_cast<int>(i), Months);
        observationDates.push_back(date);
        incorrectObservationDates.push_back(date);
        df.push_back(exp(-0.065 * dayCounter->yearFraction(anchorDate, date)));
    }
    incorrectObservationDates.push_back(calendar.advance(anchorDate, static_cast<int>(i + 1), Months));

    std::shared_ptr<DiscountCurve> dc = std::make_shared<DiscountCurve>(
        anchorDate, incorrectObservationDates, df, ArrayInterpolatorType::LINEAR, allowExtrapolation);
    BOOST_REQUIRE(dc->isOK() == false);

    vector<string> msgs = dc->getErrorMessages();
    BOOST_CHECK(msgs.size() == 1);

    std::shared_ptr<DiscountCurve> dc2 = std::make_shared<DiscountCurve>(
        anchorDate, observationDates, df, ArrayInterpolatorType::LINEAR, allowExtrapolation);
    BOOST_CHECK(dc2->isOK() == true);
    BOOST_CHECK(dc2->getAnchorDate() == anchorDate);
    Date d1 = calendar.advance(anchorDate, 5, Months);
    Date d2 = calendar.advance(anchorDate, 13, Months);
    Date d3 = calendar.advance(anchorDate, -1, Months);
    BOOST_CHECK(dc2->isInRange(d1) == true);
    BOOST_CHECK(dc2->isInRange(d2) == false);
    BOOST_CHECK(dc2->isInRange(d3) == false);
    // Check the behaviour on the anchor date
    BOOST_CHECK(abs(dc2->getDiscountFactor(anchorDate) - 1.0) < EPSILON_FOR_DOUBLES);

    allowExtrapolation = !allowExtrapolation;
    dc2 = std::make_shared<DiscountCurve>(anchorDate, observationDates, df, ArrayInterpolatorType::LINEAR,
                                          allowExtrapolation);
    BOOST_CHECK(dc2->isInRange(d1) == true);
    BOOST_CHECK(dc2->isInRange(d2) == true);
    BOOST_CHECK(dc2->isInRange(d3) == false);
    // Check the behaviour on the anchor date
    BOOST_CHECK(abs(dc2->getDiscountFactor(anchorDate) - 1.0) < EPSILON_FOR_DOUBLES);
}

void DiscountCurvesTest::testRatesEnvironmentValidationReport()
{
    Date anchorDate(27, Jan, 2012);
    RatesEnvironment emptyEnvironment(anchorDate);

    ValidationReport snapshot = emptyEnvironment.validationReport();
    BOOST_CHECK(snapshot.isOK());
    BOOST_CHECK(snapshot.getIssues().empty());

    ValidationReport emptyReport = emptyEnvironment.validateReport();
    BOOST_REQUIRE(!emptyReport.isOK());
    BOOST_REQUIRE(emptyReport.getIssues().size() == 1);
    BOOST_CHECK(emptyReport.getIssues()[0].code == "RATES_ENVIRONMENT_NO_RATES");
    BOOST_REQUIRE(emptyReport.getIssues()[0].path.size() == 1);
    BOOST_CHECK(emptyReport.getIssues()[0].path[0] == "rates_environment");
    BOOST_CHECK(emptyReport.getIssues()[0].message == "No rates set");
    BOOST_CHECK(emptyEnvironment.getErrorMessagesAsString() == "RatesEnvironment: No rates set");

    Calendar calendar = SouthAfrica();
    vector<Date> observationDates;
    vector<double> discountFactors;
    for (size_t i = 1; i <= 3; ++i)
    {
        observationDates.push_back(calendar.advance(anchorDate, static_cast<int>(i), Months));
        discountFactors.push_back(0.99 - i * 0.01);
    }
    observationDates.push_back(calendar.advance(anchorDate, 4, Months));

    std::shared_ptr<DiscountCurve> invalidCurve(std::make_shared<DiscountCurve>(
        anchorDate, observationDates, discountFactors, ArrayInterpolatorType::LINEAR, false));
    RatesEnvironment invalidEnvironment(anchorDate);
    invalidEnvironment.setDRS(invalidCurve);
    ValidationReport invalidReport = invalidEnvironment.validateReport();
    BOOST_REQUIRE(!invalidReport.isOK());
    BOOST_REQUIRE(invalidReport.getIssues().size() == 1);
    BOOST_CHECK(invalidReport.getIssues()[0].code == "DISCOUNT_CURVE_DIMENSION_MISMATCH");
    BOOST_REQUIRE(invalidReport.getIssues()[0].path.size() == 4);
    BOOST_CHECK(invalidReport.getIssues()[0].path[0] == "rates_environment");
    BOOST_CHECK(invalidReport.getIssues()[0].path[1] == "discount_rate_source");
    BOOST_CHECK(invalidReport.getIssues()[0].path[2] == "discount_curve");
    BOOST_CHECK(invalidReport.getIssues()[0].path[3] == "inputs");
}

void DiscountCurvesTest::testParallelBump()
{
    BOOST_TEST_MESSAGE("Testing Discount Curve Parallel Bump ...");
    Calendar calendar = SouthAfrica();
    Date anchorDate = Date(27, Jan, 2012);
    vector<Date> observationDates;
    vector<double> df;
    std::shared_ptr<DayCounter> dayCounter = std::make_shared<Actual365Fixed>();
    bool allowExtrapolation = false;
    size_t i;
    for (i = 1; i <= 12; ++i)
    {
        Date date = calendar.advance(anchorDate, static_cast<int>(i), Months);
        observationDates.push_back(date);
        df.push_back(exp(-0.065 * dayCounter->yearFraction(anchorDate, date)));
    }
    std::shared_ptr<DiscountCurve> dc2 = std::make_shared<DiscountCurve>(
        anchorDate, observationDates, df, ArrayInterpolatorType::LINEAR, allowExtrapolation);
    std::shared_ptr<InterestRateConvention> irc =
        std::make_shared<InterestRateConvention>(dayCounter, Continuous, Annual);
    std::shared_ptr<DiscountRateSource> dc3 = dc2->parallelBump(0.01, irc);
    BOOST_REQUIRE(dc3->isOK());
    double dfTest = dc2->getDiscountFactor(observationDates[2]);
    InterestRateConvention convention(dayCounter, Continuous, Annual);
    double tmp = convention.dfToRate(dfTest, anchorDate, observationDates[2]);
    double dfPlusSPread = convention.rateToDF(tmp + 0.01, anchorDate, observationDates[2]);
    double dfCheck = dc3->getDiscountFactor(observationDates[2]);
    BOOST_CHECK(abs(dfPlusSPread - dfCheck) < EPSILON_FOR_DOUBLES);
}

void DiscountCurvesTest::testConsistencyOfGetMethods()
{
    BOOST_TEST_MESSAGE("Testing Discount Curve for consistency between get methods ...");

    GenerateRates re = GenerateRates();
    Date anchorDate = re.defaultAnchorDate;
    std::shared_ptr<Calendar> defaultCalendar = re.defaultCalendar;
    std::shared_ptr<DiscountCurve> testDiscountCurve = re.getDefaultDiscountCurve(anchorDate);

    Date fromDate = defaultCalendar->advance(anchorDate, 1, Months);
    double fromDF = testDiscountCurve->getDiscountFactor(fromDate);

    Date toDate = defaultCalendar->advance(anchorDate, 9, Months);
    double toDF = testDiscountCurve->getDiscountFactor(toDate);

    BOOST_CHECK(abs(testDiscountCurve->getDiscountFactor(fromDate, toDate) - toDF / fromDF) <
                EPSILON_FOR_DOUBLES);

    vector<Date> toDates;
    toDates.push_back(defaultCalendar->advance(anchorDate, 6, Months));
    toDates.push_back(defaultCalendar->advance(anchorDate, 7, Months));
    toDates.push_back(defaultCalendar->advance(anchorDate, 8, Months));
    toDates.push_back(defaultCalendar->advance(anchorDate, 9, Months));

    vector<double> dfs = testDiscountCurve->getDiscountFactor(toDates);
    vector<double> dfs1 = testDiscountCurve->getDiscountFactor(fromDate, toDates);
    for (size_t i = 0; i < toDates.size(); ++i)
    {
        BOOST_CHECK(abs(dfs[i] / fromDF - dfs1[i]) < EPSILON_FOR_DOUBLES);
        BOOST_CHECK(abs(testDiscountCurve->getDiscountFactor(toDates[i]) / fromDF - dfs1[i]) <
                    EPSILON_FOR_DOUBLES);
    }
}

void DiscountCurvesTest::testValues()
{
    BOOST_TEST_MESSAGE("Testing Discount Curve values ...");

    GenerateRates re = GenerateRates();
    Date defaultAnchorDate = re.defaultAnchorDate;
    std::shared_ptr<DayCounter> defaultDayCounter = re.defaultDayCounter;
    vector<Date> defaultDates = re.defaultCurveDates;
    double defaultNACCRate = re.defaultNACCRate;
    std::shared_ptr<DiscountCurve> testDiscountCurve = re.getDefaultDiscountCurve(defaultAnchorDate);

    Date date1 = defaultDates[0];

    double df = testDiscountCurve->getDiscountFactor(date1);
    double time1 = defaultDayCounter->yearFraction(defaultAnchorDate, date1);
    BOOST_CHECK(abs(df - exp(-defaultNACCRate * time1)) < EPSILON_FOR_DOUBLES);
}

void DiscountCurvesTest::testRollForward()
{
    BOOST_TEST_MESSAGE("Testing Discount Curve roll forward ...");

    GenerateRates re = GenerateRates();
    std::shared_ptr<Calendar> defaultCalendar = re.defaultCalendar;
    Date defaultAnchorDate = re.defaultAnchorDate;
    std::shared_ptr<DiscountCurve> testDiscountCurve = re.getDefaultDiscountCurve(defaultAnchorDate);

    Date rollDate = defaultCalendar->advance(defaultAnchorDate, 1, Years);
    Date checkDate = defaultCalendar->advance(defaultAnchorDate, 2, Years);

    std::shared_ptr<DiscountRateSource> rolledCurve = testDiscountCurve->rollForward(rollDate);
    BOOST_REQUIRE(rolledCurve->isOK());
    BOOST_CHECK(rolledCurve->getAnchorDate() == rollDate);

    double manualDF = testDiscountCurve->getDiscountFactor(rollDate, checkDate);
    double rolledDF = rolledCurve->getDiscountFactor(checkDate);
    BOOST_CHECK(abs(manualDF - rolledDF) < EPSILON_FOR_DOUBLES);
}

test_suite* DiscountCurvesTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("Discount Factor Curve");

    suite->add(BOOST_TEST_CASE(&DiscountCurvesTest::testConstruction));
    suite->add(BOOST_TEST_CASE(&DiscountCurvesTest::testRatesEnvironmentValidationReport));
    suite->add(BOOST_TEST_CASE(&DiscountCurvesTest::testParallelBump));
    suite->add(BOOST_TEST_CASE(&DiscountCurvesTest::testConsistencyOfGetMethods));
    suite->add(BOOST_TEST_CASE(&DiscountCurvesTest::testValues));
    suite->add(BOOST_TEST_CASE(&DiscountCurvesTest::testRollForward));
    return suite;
}
