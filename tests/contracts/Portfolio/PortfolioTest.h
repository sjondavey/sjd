#ifndef portfolio_test
#define portfolio_test

#include <iostream>

#include <math.h>
#include <boost/test/unit_test.hpp>

#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/contracts/Portfolio/RegulatoryCapital.h>

#include <support/GenerateRatesEnvironments.h>
#include <support/GenerateContracts.h>
#include <support/TestingParameters.h>
// to use the boost::math::isnan(...) function
#include <boost/math/special_functions/fpclassify.hpp>

using namespace sjd;

class PortfolioTest 
{

public:
    
    static boost::shared_ptr<RegCapUnderlyingFactors> getDefaultRegCapUnderlyingFactors();
    static boost::shared_ptr<RegCapCreditParameters> getDefaultRegCreditParameters();

    static void testStructureCreation();

    static void testPortfolioB2CEM();
    

    static boost::unit_test_framework::test_suite* suite();
    
};

#endif


