#ifndef cashflowcontract_test
#define cashflowcontract_test

#include <iostream>

#include <math.h>
#include <boost/test/unit_test.hpp>

#include <sjd/contracts/Cashflow.h>

#include <support/GenerateRatesEnvironments.h>
#include <support/GenerateContracts.h>
#include <support/TestingParameters.h>
// to use the boost::math::isnan(...) function
#include <boost/math/special_functions/fpclassify.hpp>


#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/all.hpp>

using namespace sjd;

class CashflowTest 
{
  public:

    static void testBuySell();
    static void testVolume();
    static void testMaturityDate();
    static void testSettlementDate();

    static void testValue();
    static void testGetPayoffAfterMaturity();

    static boost::unit_test_framework::test_suite* suite();
};

#endif


