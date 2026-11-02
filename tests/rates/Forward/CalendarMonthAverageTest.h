#pragma once

#include <iostream>

#include <sjd/rates/Forward/Deterministic/ForwardCurveCMADeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveCMAStochasticFwdVol.h>
#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

#include <boost/test/unit_test.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>

class CalendarMonthAverageTest
{
public:
    static void testConstruction();
    static void testValues();
    static void testRoll_CMA();

    static boost::unit_test_framework::test_suite* suite();
};
