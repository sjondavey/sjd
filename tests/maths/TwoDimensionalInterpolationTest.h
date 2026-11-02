#pragma once

#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sjd/maths/TwoDimensionalInterpolation.h>

class Maths2DInterpTest
{
public:
    static void testBilinearInterpolator();
    static void testBicubicInterpolator();

    static boost::unit_test_framework::test_suite* suite();
};
