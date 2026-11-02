#ifndef PortfolioEvolver_Test
#define PortfolioEvolver_Test

#include <iostream>

#include <boost/test/unit_test.hpp>

#include <sjd/contracts/Portfolio/PortfolioEvolver.h>

#include <support/GenerateContracts.h>
#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

using namespace sjd;

class PortfolioEvolverTest
{
public:
    static void testPortfolioEvovler();
    static boost::unit_test_framework::test_suite* suite();
};

#endif
