#include <boost/test/unit_test.hpp>
#include <iostream>
#include <ql/time/calendars/all.hpp>
#include <sjd/contracts/Portfolio/CVA/cva.h>
#include <support/TestingParameters.h>

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

BOOST_AUTO_TEST_SUITE(CVATest)

BOOST_AUTO_TEST_CASE(testFlatPD)
{
    BOOST_TEST_MESSAGE("Testing Flat PD CVA");

    Date anchorDate = Date(05, June, 2014);
    double pd = .1;
    double lgd = .45;
    FlatPD cva(anchorDate, pd, lgd);
    BOOST_REQUIRE(cva.validate().isOK());

    cva.setLGD(0);
    BOOST_CHECK(cva.validate().isOK());
    cva.setLGD(1);
    BOOST_CHECK(cva.validate().isOK());
    cva.setPD(0);
    BOOST_CHECK(cva.validate().isOK());
    cva.setPD(1);
    BOOST_CHECK(cva.validate().isOK());
    cva.setPD(1.5);
    BOOST_CHECK(!cva.validate().isOK());
    cva.setPD(-1.5);
    BOOST_CHECK(!cva.validate().isOK());
    cva.setPD(0.5);
    cva.setLGD(-1);
    BOOST_CHECK(!cva.validate().isOK());
    cva.setLGD(1.5);
    BOOST_CHECK(!cva.validate().isOK());

    cva.setLGD(lgd);
    cva.setPD(.1);
    Calendar nullCalendar = NullCalendar();

    Date d = nullCalendar.advance(anchorDate, 2, Years);
    double pd1 = cva.getPD(d);
    double manualHazardRate = -std::log(1 - pd);
    double t = (d - anchorDate) / 365.0;
    double cpd = 1 - exp(-t * manualHazardRate);
    BOOST_CHECK_SMALL(pd1 - cpd, EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testCumulativeDefaultProbability)
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
    cva.validate().isOK();
    BOOST_REQUIRE(cva.validate().isOK());
    BOOST_CHECK_SMALL(cva.getPD(anchorDate), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(cva.getLGD(anchorDate) - 0.4, EPSILON_FOR_DOUBLES);
    Date beforeFirstPillar = anchorDate + 180;
    BOOST_CHECK_SMALL(cva.getLGD(beforeFirstPillar) - 0.4, EPSILON_FOR_DOUBLES);

    Date d = nullCalendar.advance(anchorDate, 2, Years);
    double pd1 = cva.getPD(d);
    BOOST_CHECK_SMALL(pd1 - 0.02, EPSILON_FOR_DOUBLES);

    vector<double> decreasing;
    decreasing.push_back(0.05);
    decreasing.push_back(0.04);
    decreasing.push_back(0.03);
    decreasing.push_back(0.02);
    decreasing.push_back(0.01);
    CumulativeDefaultProbability falling(anchorDate, dates, decreasing, lgd);
    BOOST_CHECK(!falling.validate().isOK());
}

BOOST_AUTO_TEST_SUITE_END()
