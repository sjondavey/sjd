#pragma once

#include <boost/test/unit_test.hpp>

class ErrorsTest
{
public:
    static void testErrorCodes();
    static void testValidationErrorCarriesTheReport();
    static void testRequireAndAssert();
    static void testConfigurationSites();
    static void testReportHelpers();

    static boost::unit_test_framework::test_suite* suite();
};
