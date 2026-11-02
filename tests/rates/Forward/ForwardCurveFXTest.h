#pragma once

#include <iostream>

#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministicFX.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFX.h>

#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

#include <boost/test/unit_test.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>

class ForwardCurveFXTest
{
public:
    static void getDefaultDatesAndDF(QuantLib::Date anchorDate,
                                     std::shared_ptr<DiscountCurve>& domesticDRS,
                                     std::shared_ptr<DiscountCurve>& foreignDRS,
                                     std::shared_ptr<VolatilityCurve>& vrs);

    static void testForwardCurveConstruction();
    static void testDeterministicCurve();
    static void testStochasticCurve();

    static boost::unit_test_framework::test_suite* suite();
};
