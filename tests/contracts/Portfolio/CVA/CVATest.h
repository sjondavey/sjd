#ifndef cva_test
#define cva_test

#include <iostream>

#include <sjd/contracts/Portfolio/CVA/cva.h>
#include <support/TestingParameters.h>

#include <boost/test/unit_test.hpp>
#include <ql/time/calendars/all.hpp>

using namespace sjd;

class CVATest
{
public:
    static void testFlatPD();
    static void testCumulativeDefaultProbability();

    static boost::unit_test_framework::test_suite* suite();
};

#endif
