#ifndef sjddiscountcurves_test_maths
#define sjddiscountcurves_test_maths

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>
#include <sjd/rates/Discount/DiscountCurves.h>

class DiscountCurvesTest 
{
  public:

    static void testConstruction();
    static void testRatesEnvironmentValidationReport();
    static void testParallelBump();

    static void testConsistencyOfGetMethods(); 
    static void testValues();
    static void testRollForward();

    static boost::unit_test_framework::test_suite* suite();
};

#endif
