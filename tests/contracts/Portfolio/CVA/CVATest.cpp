#include "CVATest.h"

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

void CVATest::testFlatPD()
{
    BOOST_TEST_MESSAGE("Testing Flat PD CVA");

    Date anchorDate = Date(05, June, 2014);
    double pd = .1;
    double lgd = .45;
    FlatPD cva(anchorDate, pd, lgd);
    BOOST_REQUIRE(cva.isOK());

    cva.setLGD(0);
    BOOST_CHECK(cva.isOK());
    cva.setLGD(1);
    BOOST_CHECK(cva.isOK());
    cva.setPD(0);
    BOOST_CHECK(cva.isOK());
    cva.setPD(1);
    BOOST_CHECK(cva.isOK());
    cva.setPD(1.5);
    BOOST_CHECK(!cva.isOK());
    cva.setPD(-1.5);
    BOOST_CHECK(!cva.isOK());
    cva.setPD(0.5);
    cva.setLGD(-1);
    BOOST_CHECK(!cva.isOK());
    cva.setLGD(1.5);
    BOOST_CHECK(!cva.isOK());

    cva.setLGD(lgd);
    cva.setPD(.1);
    Calendar nullCalendar = NullCalendar();

    Date d = nullCalendar.advance(anchorDate, 2, Years);
    double pd1 = cva.getPD(d);
    double manualHazardRate = -std::log(1 - pd);
    double t = (d - anchorDate) / 365.0;
    double cpd = 1 - exp(-t * manualHazardRate);
    BOOST_CHECK(abs(pd1 - cpd) < EPSILON_FOR_DOUBLES);
}

void CVATest::testCumulativeDefaultProbability()
{
    BOOST_TEST_MESSAGE("Testing CumulativeDefaultProbability CVA");

    Date anchorDate = Date(05, June, 2014);

    Calendar nullCalendar = NullCalendar();

    vector<Date> dates;
    vector<double> cdp, lgd;
    for (size_t i = 1; i < 6; ++i)
    {
        dates.push_back(nullCalendar.advance(anchorDate, static_cast<int>(i), Years));
        cdp.push_back(0.01 * i);
        lgd.push_back(0.4);
    }

    CumulativeDefaultProbability cva(anchorDate, dates, cdp, lgd);
    cva.isOK();
    BOOST_REQUIRE(cva.isOK());

    Date d = nullCalendar.advance(anchorDate, 2, Years);
    double pd1 = cva.getPD(d);
    BOOST_CHECK(abs(pd1 - 0.02) < EPSILON_FOR_DOUBLES);
}

test_suite* CVATest::suite()
{

    test_suite* suite = BOOST_TEST_SUITE("CVA");
    suite->add(BOOST_TEST_CASE(&CVATest::testFlatPD));
    suite->add(BOOST_TEST_CASE(&CVATest::testCumulativeDefaultProbability));

    return suite;
}
