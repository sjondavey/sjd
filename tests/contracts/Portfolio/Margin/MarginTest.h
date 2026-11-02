#ifndef margin_test
#define margin_test

#include <iostream>

#include <sjd/contracts/Portfolio/Margin/Margin.h>
#include <support/TestingParameters.h>

#include <boost/test/unit_test.hpp>

using namespace sjd;

class MarginTest
{
public:
    static void testSimpleMargin();

    static boost::unit_test_framework::test_suite* suite();
};

#endif
