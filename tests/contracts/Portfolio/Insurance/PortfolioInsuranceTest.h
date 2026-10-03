#ifndef portfolioinsurance_test
#define portfolioinsurance_test

#include <iostream>

#include <sjd/contracts/Portfolio/Insurance/PortfolioInsurance.h>
#include <support/TestingParameters.h>

#include <boost/test/unit_test.hpp>

using namespace sjd;

class PortfolioInsuranceTest 
{
  public:

    static void testSimpleInsurance();

    static void testSimpleInsuranceWithTermStructure();

    static boost::unit_test_framework::test_suite* suite();
};

#endif


