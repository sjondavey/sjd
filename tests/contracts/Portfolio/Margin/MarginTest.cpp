#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sjd/contracts/Portfolio/Margin/Margin.h>
#include <support/TestingParameters.h>

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

BOOST_AUTO_TEST_SUITE(MarginTest)

BOOST_AUTO_TEST_CASE(testSimpleMargin)
{
    BOOST_TEST_MESSAGE("Testing SimpleMargin");

    bool useMyThreshold = true, useCptyThreshold = true;
    double myThreshold = 2000000.0, cptyThreshold = 1000000.0, minimumTransferAmount = 500000.0,
           rounding = 10000.0;

    //
    Date anchorDate = Date(27, Jan, 2013);
    SimpleMargin margin(anchorDate, useMyThreshold, myThreshold, useCptyThreshold, cptyThreshold,
                        minimumTransferAmount, rounding);
    margin.setMarginBalance(0);
    double mtm = -950000.0;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - 0, EPSILON_FOR_DOUBLES);
    margin.updateBalanceForGivenMtM(mtm);
    mtm = -1, 450, 000.00;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - 0, EPSILON_FOR_DOUBLES);
    margin.updateBalanceForGivenMtM(mtm);
    mtm = -1515469;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - (-520000.0), EPSILON_FOR_DOUBLES);
    margin.updateBalanceForGivenMtM(mtm);
    mtm = -5100655;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - (-3590000), EPSILON_FOR_DOUBLES);
    margin.updateBalanceForGivenMtM(mtm);
    mtm = -2692350;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - 2410000.0, EPSILON_FOR_DOUBLES);
    margin.updateBalanceForGivenMtM(mtm);
    mtm = -1192350;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - 1500000.0, EPSILON_FOR_DOUBLES);
    margin.updateBalanceForGivenMtM(mtm);
    mtm = -992350;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - 200000.0, EPSILON_FOR_DOUBLES);

    margin.setMarginBalance(0);
    mtm = -9500000.0;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - (-8500000.0), EPSILON_FOR_DOUBLES);
    margin.updateBalanceForGivenMtM(mtm);
    mtm = 1700000.0;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - 8500000.0, EPSILON_FOR_DOUBLES);
    margin.updateBalanceForGivenMtM(mtm);
    mtm = 2700000;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - (700000.0), EPSILON_FOR_DOUBLES);
    margin.updateBalanceForGivenMtM(mtm);
    mtm = -1700000;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - (-1400000), EPSILON_FOR_DOUBLES);
    margin.updateBalanceForGivenMtM(mtm);
    mtm = 2750000.0;
    BOOST_CHECK_SMALL(margin.getMarginCall(mtm) - 1450000.0, EPSILON_FOR_DOUBLES);

    useMyThreshold = false;
    SimpleMargin margin2(anchorDate, useMyThreshold, myThreshold, useCptyThreshold, cptyThreshold,
                         minimumTransferAmount, rounding);
    mtm = -9500000.0;
    BOOST_CHECK_SMALL(margin2.getMarginCall(mtm) - (-8500000.0), EPSILON_FOR_DOUBLES);
    margin2.updateBalanceForGivenMtM(mtm);
    mtm = 1700000.0;
    BOOST_CHECK_SMALL(margin2.getMarginCall(mtm) - 8500000.0, EPSILON_FOR_DOUBLES);
    margin2.updateBalanceForGivenMtM(mtm);
    mtm = 2700000;
    BOOST_CHECK_SMALL(margin2.getMarginCall(mtm) - 0, EPSILON_FOR_DOUBLES);
    margin2.updateBalanceForGivenMtM(mtm);
    mtm = -1700001;
    BOOST_CHECK_SMALL(margin2.getMarginCall(mtm) - (-710000.0), EPSILON_FOR_DOUBLES);
    margin2.updateBalanceForGivenMtM(mtm);
    mtm = 2750000.0;
    BOOST_CHECK_SMALL(margin2.getMarginCall(mtm) - 710000.0, EPSILON_FOR_DOUBLES);

    // Special Case
    useMyThreshold = true;
    myThreshold = 0;
    useCptyThreshold = true;
    cptyThreshold = 0;
    minimumTransferAmount = 250000.0;
    rounding = 10000;
    SimpleMargin margin3(anchorDate, useMyThreshold, myThreshold, useCptyThreshold, cptyThreshold,
                         minimumTransferAmount, rounding);
    margin3.setMarginBalance(980000.0);
    mtm = 557141.0;
    BOOST_CHECK_SMALL(margin3.getMarginCall(mtm) - (-420000.0), EPSILON_FOR_DOUBLES);
}

BOOST_AUTO_TEST_SUITE_END()
