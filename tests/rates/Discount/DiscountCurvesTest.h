#pragma once

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sjd/rates/Discount/DiscountCurves.h>
#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

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
