#ifndef sjd_interestrateconvention_test
#define sjd_interestrateconvention_test

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <sjd/rates/Discount/InterestRateConvention.h>
#include <support/TestingParameters.h>

class InterestRateConventionTest
{
public:
    static void testDFToRateConversion();

    static boost::unit_test_framework::test_suite* suite();
};

#endif
