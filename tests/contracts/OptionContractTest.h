#ifndef optioncontract_test
#define optioncontract_test

#include <iostream>

#include <sjd/contracts/EuropeanForwardContract.h>
#include <sjd/contracts/EuropeanOptionContract.h>
#include <sjd/contracts/AsianOptionContract.h>

#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

#include <boost/test/unit_test.hpp>
// to use the boost::math::isnan(...) function
#include <boost/math/special_functions/fpclassify.hpp>

#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/all.hpp>

using namespace sjd;

class OptionContactTest 
{
  public:

    static boost::shared_ptr<EuropeanOption> getDefaultEuropeanOption(QuantLib::Date anchorDate);

    static void testBuySell();
    static void testStrike();
    static void testVolume();
    static void testMaturityDate();
    static void testPremiumAmount();
    static void testType();

    static void testValue();
    static void testGetPayoffAfterMaturity();    

    static void testPutCallParity();

    static void testEuropeanOptionWithMoneynessVol();

    static boost::unit_test_framework::test_suite* suite();
};

#endif

