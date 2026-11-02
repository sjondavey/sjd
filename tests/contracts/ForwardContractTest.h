#pragma once

#include <iostream>

#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <boost/test/unit_test.hpp>
#include <math.h>
// to use the boost::math::isnan(...) function
#include <boost/math/special_functions/fpclassify.hpp>

#include <sjd/contracts/AsianForwardContract.h>
#include <sjd/contracts/EuropeanForwardContract.h>

#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/all.hpp>

using namespace sjd;

class ForwardContactTest
{
public:
    // Returns a EuropeanForward which matures 2 years after the anchor date
    // using a null calendar
    static std::shared_ptr<EuropeanForward> getDefaultEuropeanForward(QuantLib::Date anchorDate);
    static std::shared_ptr<AsianForward> getDefaultAsianForward(QuantLib::Date anchorDate);

    static void testBuySell();
    static void testStrike();
    static void testVolume();
    static void testMaturityDate();
    static void testSettlementDate();
    static void testValidationReport();
    static void testValue();
    static void testGetPayoffAfterMaturity();

    static void testAsianAveragingDates();
    static void testFixingAndForwardDates();
    static void testAsianGetPayoffAfterMaturity();

    static void testAsianForward();

    static boost::unit_test_framework::test_suite* suite();
};
