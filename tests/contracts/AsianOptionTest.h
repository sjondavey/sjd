#ifndef asianoptioncontract_test
#define asianoptioncontract_test

#include <iostream>

#include <sjd/contracts/AsianOptionContract.h>
#include <sjd/contracts/EuropeanForwardContract.h>
#include <sjd/contracts/AsianForwardContract.h>

#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

#include <boost/test/unit_test.hpp>
#include <boost/math/special_functions/fpclassify.hpp> 

#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/all.hpp>

using namespace sjd;

class AsianOptionContactTest 
{
  public:

    static boost::shared_ptr<AsianOption> getDefaultAsianOption(QuantLib::Date anchorDate);
    static boost::shared_ptr<AsianOption> getAsianOptionWithMoneynessVol(QuantLib::Date anchorDate);


    static void testMaturityDate();
    static void testAveragingDates();
    static void testType();

    static void testValue();
    static void testGetPayoffAfterMaturity();    
    static void testPutCallParity();

    static void testEuropeanOptionWithMoneynessVol();

    static boost::unit_test_framework::test_suite* suite();
};

#endif

