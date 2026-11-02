#pragma once

#include <boost/test/unit_test.hpp>

// Values for classes that Python cannot reach yet, recorded at the
// pre-modernisation baseline. The Python golden tests cover everything else.
//
// After an intended change, run sjdTest with SJD_PRINT_GOLDEN=1 set, paste the
// printed table over the expected values, and explain the change in the commit.
class GoldenValuesTest
{
public:
    static void testContractStressor();
    static void testSimpleInsurance();

    static boost::unit_test_framework::test_suite* suite();
};
