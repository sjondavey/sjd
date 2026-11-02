#pragma once

#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sjd/pricing/Black76Formula.h>
#include <support/TestingParameters.h>

class Black76Test
{
public:
    static void testPutCallParity();

    /*
    static void testErrorMessages();
    */

    static boost::unit_test_framework::test_suite* suite();
};
