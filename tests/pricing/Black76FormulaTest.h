#ifndef black76_test_maths
#define black76_test_maths

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <support/TestingParameters.h>
#include <sjd/pricing/Black76Formula.h>

class Black76Test {
  public:

    static void testPutCallParity();

    /*
    static void testErrorMessages();
    */

    static boost::unit_test_framework::test_suite* suite();
};

#endif


