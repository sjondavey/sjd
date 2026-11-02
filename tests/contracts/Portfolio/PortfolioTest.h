#pragma once

#include <iostream>

#include <boost/test/unit_test.hpp>
#include <math.h>

#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/contracts/Portfolio/RegulatoryCapital.h>

#include <support/GenerateContracts.h>
#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>
// to use the boost::math::isnan(...) function
#include <boost/math/special_functions/fpclassify.hpp>

using namespace sjd;

class PortfolioTest
{

public:
    static std::shared_ptr<RegCapUnderlyingFactors> getDefaultRegCapUnderlyingFactors();
    static std::shared_ptr<RegCapCreditParameters> getDefaultRegCreditParameters();

    static void testStructureCreation();

    static void testPortfolioB2CEM();
    static void testInvalidRegulatoryCapitalIsNaN();
    static void testNullMarginAndUnknownFactors();

    static boost::unit_test_framework::test_suite* suite();
};
