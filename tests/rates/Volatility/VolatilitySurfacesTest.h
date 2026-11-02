#ifndef sjdvolatilitysurfaces_test_maths
#define sjdvolatilitysurfaces_test_maths

#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <ql/time/calendar.hpp>
#include <ql/time/calendars/all.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/all.hpp>
#include <sjd/rates/Volatility/VolatilitySurfaces.h>
#include <support/TestingParameters.h>

#include <support/GenerateRatesEnvironments.h>

class VolatilitySurfacesTest
{
public:
    static void testEuropeanMoneynessConstruction();
    static void testEuropeanMoneynessConsistencyBetweenVolAndSD();
    static void testEuropeanMoneynessRoll();

    static void testCalendarMonthMoneynessConstruction();
    static void testCalendarMonthMoneynessConsistencyBetweenVolAndSD();
    static void testCalendarMonthMoneynessRoll();

    static void testForwardDeltaVolatilitySurfaceConstruction();

    static void testFlatWings();

    static boost::unit_test_framework::test_suite* suite();
};

#endif
