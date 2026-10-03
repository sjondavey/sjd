#ifndef sjdfixingrate_test
#define sjdfixingrate_test

#include <iostream>
#include <sjd/rates/Fixing/FixingCurves.h>
#include <support/TestingParameters.h>

#include <boost/test/unit_test.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/calendars/nullcalendar.hpp>


class FixingRateTest 
{
  public:
    static void testDailyFixingCurve();
    static void testMonthlyFixingCurve();

    static void testHistoricFedFundsCurve();

    static boost::unit_test_framework::test_suite* suite();
};

#endif
