#ifndef sjdvolatilitycurve_test_maths
#define sjdvolatilitycurve_test_maths

#include <boost/test/unit_test.hpp>
#include <iostream>

#include <support/GenerateRatesEnvironments.h>

#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <support/TestingParameters.h>

class VolatilityCurveTest
{
public:
    static void testVolatilityCurveConstruction();
    static void testVolatilityCurveConsistencyBetweenVolAndSD();
    static void testVolatilityCurveValues();
    static void testVolatilityCurveRoll();

    static boost::unit_test_framework::test_suite* suite();
};

#endif
