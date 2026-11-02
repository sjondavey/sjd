#ifndef sjdfuturescurve_test_maths
#define sjdfuturescurve_test_maths

#include <iostream>
#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFwdVol.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticSpotVol.h>

#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

#include <boost/test/unit_test.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>

class ForwardCurveTest
{
public:
    static void testForwardCurveConstruction();

    static void testConsistencyBetweenGetMethods();
    static void testValues();
    static void testRoll();

    static void testForwardCurveRollsAtForwardVolConstruction();
    static void testRollStochastic();
    static void testSpotVolPathVariance();

    static boost::unit_test_framework::test_suite* suite();
};

#endif
