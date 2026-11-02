#pragma once

#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sjd/maths/maths.h>

class MathsFunctionsTest
{
public:
    static void testFunctions();
    static void testLinearInterpolator();
    // Test some of the functionality of the abstract base class using one instance
    static void testArrayInterpolator();
    static void testLinearArrayInterpolator();
    static void testCubicSplineInterpolator();

    static boost::unit_test_framework::test_suite* suite();
};
