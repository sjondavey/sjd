
#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sjd/pricing/Black76Formula.h>
#include <sjd/tools/Errors.h>
#include <support/TestingParameters.h>

using namespace std;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;

/*

void BlackScholesTest::testErrorMessages() {
    BOOST_TEST_MESSAGE("Testing Black Scholes Error Messages ...");

    double F = 11.1;
    double X = 11.0;
    double yte = 1.3;
    double yts = 1.3;
    double vol = 0.2;
    double sd = vol * sqrt(yte);
    double r = 0.065;
    double df = exp(-r * yts);

    EuropeanCallBlack76 call(F, X, yte, yts, sd, df);
    EuropeanPutBlack76 put(F, X, yte, yts, sd, df);

    call.setForward(-1.0);
    BOOST_CHECK(call.isOK() == false);
    string expectedErrorMessage = "EuropeanCallBlack76->EuropeanOptionBlack76->BlackInputs->Forward is <= 0";
    BOOST_CHECK(call.getErrorMessage().compare(expectedErrorMessage) == 0);
    call.setForward(F);

    put.setStandardDeviation(0);
    BOOST_CHECK(put.isOK() == false);
    expectedErrorMessage = "EuropeanPutBlack76->EuropeanOptionBlack76->BlackInputs->Standard Deviation is <=
0"; BOOST_CHECK(put.getErrorMessage().compare(expectedErrorMessage) == 0);

}

*/

BOOST_AUTO_TEST_SUITE(Black76Test)

BOOST_AUTO_TEST_CASE(testPutCallParity)
{
    BOOST_TEST_MESSAGE("Testing Black Scholes Inputs ...");

    double F = 100, X = 110, sd = 0.2, df = 0.97;
    Black76Call call(F, X, sd, df);
    Black76Put put(F, X, sd, df);
    BOOST_CHECK_THROW(Black76Call(-1, X, sd, df), ConfigurationError);
    BOOST_CHECK_THROW(Black76Put(F, X, 0, df), ConfigurationError);

    Black76Option* option = new Black76Call(F, X, sd, df);

    // Put call parity
    BOOST_CHECK_SMALL(put.getPremium() - call.getPremium() - (X - F) * df, EPSILON_FOR_DOUBLES);

    delete option;
}

BOOST_AUTO_TEST_SUITE_END()
