#include "AsianOptionFormulaTest.h"

using namespace std;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;

/*======================================================================================
AsianOptionInputParameters
=======================================================================================*/
AsianOptionInputParameters::AsianOptionInputParameters(vector<double> time,
                                                       vector<double> forwardRates,
                                                       double strikeInput,
                                                       double yte,
                                                       double yts,
                                                       vector<double> standardDeviation,
                                                       double discountFactorToSettlement)
{
    N = forwardRates.size();
    M = 0;
    size_t i = 0;
    while ((i < N) && (time[i] <= 0))
    {
        ++M;
        fixes.push_back(forwardRates[i]);
        ++i;
    }
    while (i < N)
    {
        fwds.push_back(forwardRates[i]);
        sds.push_back(standardDeviation[i]);
        ++i;
    }
    strike = strikeInput;
    df = discountFactorToSettlement;
}

AsianOptionInputParameters::AsianOptionInputParameters(vector<double> forwardRates, // "Live" forwards
                                                       vector<double> fixedRates,
                                                       double strikeInput,
                                                       vector<double> standardDeviation,
                                                       double discountFactorToSettlement)
{
    fwds = forwardRates;
    sds = standardDeviation;
    fixes = fixedRates;
    strike = strikeInput;
    df = discountFactorToSettlement;
    N = fixes.size() + fwds.size();
    M = fixes.size();
}

/*======================================================================================
AsianOptionTest
=======================================================================================*/
void AsianOptionTest::testErrorMessages()
{
    BOOST_TEST_MESSAGE("Testing Asian Option Error Messages ...");
    BOOST_TEST_MESSAGE("... before averaging");
    testErrorMessagesWithSpecificData(getAsianInputsBeforeAveragingStarts());
    BOOST_TEST_MESSAGE("... during averaging");
    testErrorMessagesWithSpecificData(getAsianInputsDuringAveraging());
    BOOST_TEST_MESSAGE("... during averaging with high average");
    testErrorMessagesWithSpecificData(getAsianInputsJustBeforeSettlementWithHighAverage());
    BOOST_TEST_MESSAGE("... during averaging with low average");
    testErrorMessagesWithSpecificData(getAsianInputsJustBeforeSettlementWithLowAverage());
}

void AsianOptionTest::testErrorMessagesWithSpecificData(AsianOptionInputParameters ip)
{

    AverageRateCall call(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    AverageRatePut put(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    BOOST_CHECK(call.isOK() == true);
    BOOST_CHECK(put.isOK() == true);

    // input vectors must have the correct dimension
    vector<double> incorrectFwds = ip.fwds;
    incorrectFwds.pop_back();
    put.setParameters(incorrectFwds, ip.fixes, ip.strike, ip.sds, ip.df);
    BOOST_CHECK(put.isOK() == false);
    vector<double> incorrectSD = ip.sds;
    incorrectSD.pop_back();
    put.setParameters(incorrectFwds, ip.fixes, ip.strike, incorrectSD, ip.df);
    BOOST_CHECK(put.isOK() == true);
    put.setParameters(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
}

void AsianOptionTest::testValulationBeforeAveraging()
{
    BOOST_TEST_MESSAGE("Testing Asian Option Valuation before averaging");
    AsianOptionInputParameters ip = getAsianInputsBeforeAveragingStarts();
    AverageRateCall call(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    AverageRatePut put(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    BOOST_REQUIRE(call.isOK() == true);
    BOOST_REQUIRE(put.isOK() == true);

    // Manual replication
    double effectiveStrike = ip.strike;
    double effectiveForward = 0;
    for (size_t i = 0; i < ip.N; ++i)
    {
        effectiveForward += ip.fwds[i];
    }
    effectiveForward /= ip.N;

    double NSquared = ip.N * ip.N;
    double effectiveStandardDeviation = 0;
    for (size_t j = 0; j < ip.N; ++j)
    {
        for (size_t i = j; i < ip.N; ++i)
        {
            effectiveStandardDeviation += 2 * ip.fwds[i] * ip.fwds[j] / NSquared * exp(ip.sds[j] * ip.sds[j]);
        }
        effectiveStandardDeviation -= ip.fwds[j] * ip.fwds[j] / NSquared * exp(ip.sds[j] * ip.sds[j]);
    }
    effectiveStandardDeviation = std::log(effectiveStandardDeviation / (effectiveForward * effectiveForward));
    effectiveStandardDeviation = sqrt(effectiveStandardDeviation);
    Black76Call equivalentCall(effectiveForward, effectiveStrike, effectiveStandardDeviation, ip.df);
    Black76Put equivalentPut(effectiveForward, effectiveStrike, effectiveStandardDeviation, ip.df);

    BOOST_CHECK(abs(call.getPremium() - equivalentCall.getPremium()) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(put.getPremium() - equivalentPut.getPremium()) < EPSILON_FOR_DOUBLES);
}
void AsianOptionTest::testValulationDuringAveraging()
{
    BOOST_TEST_MESSAGE("Testing Asian Option Valuation during averaging");
    AsianOptionInputParameters ip = getAsianInputsDuringAveraging();
    AverageRateCall call(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    AverageRatePut put(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    BOOST_REQUIRE(call.isOK() == true);
    BOOST_REQUIRE(put.isOK() == true);

    // Manual replication
    double effectiveStrike = 0;
    for (size_t i = 0; i < ip.M; ++i)
    {
        effectiveStrike += ip.fixes[i];
    }
    effectiveStrike /= ip.N;
    effectiveStrike = -effectiveStrike;
    effectiveStrike += ip.strike;

    double effectiveForward = 0;
    for (size_t i = 0; i < ip.N - ip.M; ++i)
    {
        effectiveForward += ip.fwds[i];
    }
    effectiveForward /= ip.N;

    double NSquared = ip.N * ip.N;
    double effectiveStandardDeviation = 0;
    for (size_t j = 0; j < ip.N - ip.M; ++j)
    {
        for (size_t i = j; i < ip.N - ip.M; ++i)
        {
            effectiveStandardDeviation += 2 * ip.fwds[i] * ip.fwds[j] / NSquared * exp(ip.sds[j] * ip.sds[j]);
        }
        effectiveStandardDeviation -= ip.fwds[j] * ip.fwds[j] / NSquared * exp(ip.sds[j] * ip.sds[j]);
    }
    effectiveStandardDeviation = std::log(effectiveStandardDeviation / (effectiveForward * effectiveForward));
    effectiveStandardDeviation = sqrt(effectiveStandardDeviation);
    Black76Call equivalentCall(effectiveForward, effectiveStrike, effectiveStandardDeviation, ip.df);
    Black76Put equivalentPut(effectiveForward, effectiveStrike, effectiveStandardDeviation, ip.df);

    BOOST_CHECK(abs(call.getPremium() - equivalentCall.getPremium()) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(put.getPremium() - equivalentPut.getPremium()) < EPSILON_FOR_DOUBLES);
}

void AsianOptionTest::testValulationJustBeforeSettlementWithHighAverage()
{
    BOOST_TEST_MESSAGE("Testing Asian Option Valuation during averaging with high average");

    AsianOptionInputParameters ip = getAsianInputsJustBeforeSettlementWithHighAverage();
    AverageRateCall call(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    AverageRatePut put(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    BOOST_REQUIRE(call.isOK() == true);
    BOOST_REQUIRE(put.isOK() == true);

    // Manual replication
    double effectiveStrike = 0;
    for (size_t i = 0; i < ip.M; ++i)
    {
        effectiveStrike += ip.fixes[i];
    }
    effectiveStrike /= ip.N;
    effectiveStrike = -effectiveStrike;
    effectiveStrike += ip.strike;

    double effectiveForward = 0;
    for (size_t i = 0; i < ip.N - ip.M; ++i)
    {
        effectiveForward += ip.fwds[i];
    }
    effectiveForward /= ip.N;

    double amountByWhichAverageExceedsStrike = -effectiveStrike;
    amountByWhichAverageExceedsStrike += effectiveForward;

    double equivalentCallPremium = amountByWhichAverageExceedsStrike * ip.df;
    double equivalentPutPremium = 0;

    BOOST_CHECK(abs(call.getPremium() - equivalentCallPremium) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(put.getPremium() - equivalentPutPremium) < EPSILON_FOR_DOUBLES);
}

void AsianOptionTest::testValulationJustBeforeSettlementWithLowAverage()
{
    BOOST_TEST_MESSAGE("Testing Asian Option Valuation during averaging with low average");
    AsianOptionInputParameters ip = getAsianInputsJustBeforeSettlementWithLowAverage();
    AverageRateCall call(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    AverageRatePut put(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    BOOST_REQUIRE(call.isOK() == true);
    BOOST_REQUIRE(put.isOK() == true);

    // Manual replication
    double effectiveStrike = 0;
    for (size_t i = 0; i < ip.M; ++i)
    {
        effectiveStrike += ip.fixes[i];
    }
    effectiveStrike /= ip.N;
    effectiveStrike = -effectiveStrike;
    effectiveStrike += ip.strike;

    double effectiveForward = 0;
    for (size_t i = 0; i < ip.N - ip.M; ++i)
    {
        effectiveForward += ip.fwds[i];
    }
    effectiveForward /= ip.N;

    double NSquared = ip.N * ip.N;
    double effectiveStandardDeviation = 0;
    for (size_t j = 0; j < ip.N - ip.M; ++j)
    {
        for (size_t i = j; i < ip.N - ip.M; ++i)
        {
            effectiveStandardDeviation += 2 * ip.fwds[i] * ip.fwds[j] / NSquared * exp(ip.sds[j] * ip.sds[j]);
        }
        effectiveStandardDeviation -= ip.fwds[j] * ip.fwds[j] / NSquared * exp(ip.sds[j] * ip.sds[j]);
    }
    effectiveStandardDeviation = std::log(effectiveStandardDeviation / (effectiveForward * effectiveForward));
    effectiveStandardDeviation = sqrt(effectiveStandardDeviation);
    Black76Call equivalentCall(effectiveForward, effectiveStrike, effectiveStandardDeviation, ip.df);
    Black76Put equivalentPut(effectiveForward, effectiveStrike, effectiveStandardDeviation, ip.df);

    BOOST_CHECK(abs(call.getPremium() - equivalentCall.getPremium()) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(put.getPremium() - equivalentPut.getPremium()) < EPSILON_FOR_DOUBLES);
}

test_suite* AsianOptionTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("Asian Option Pricing Suite");

    suite->add(BOOST_TEST_CASE(&AsianOptionTest::testErrorMessages));

    suite->add(BOOST_TEST_CASE(&AsianOptionTest::testValulationBeforeAveraging));
    suite->add(BOOST_TEST_CASE(&AsianOptionTest::testValulationDuringAveraging));
    suite->add(BOOST_TEST_CASE(&AsianOptionTest::testValulationJustBeforeSettlementWithHighAverage));
    suite->add(BOOST_TEST_CASE(&AsianOptionTest::testValulationJustBeforeSettlementWithLowAverage));
    return suite;
}

AsianOptionInputParameters AsianOptionTest::getAsianInputsBeforeAveragingStarts()
{
    vector<double> time, fwds, sds;
    double strike, yte, yts, df;

    double spot = 21.13;
    strike = 22.0;
    double rfr = 0.065, yield = 0.025;
    double vol = 0.25;
    size_t i;
    for (i = 366; i < 386; ++i) // start of averaging is about a year from now for 20 days
    {
        double yearsAhead = (double)i / 365.0;
        time.push_back(yearsAhead);
        fwds.push_back(spot * exp(-(rfr - yield) * (yearsAhead + 2.0 / 365.0))); // T+2 convention for fwds
        sds.push_back(vol * sqrt(yearsAhead));
    }
    yte = (double)i / 365.0;
    yts = (double)(i + 2) / 365.0;
    df = exp(-rfr * yts);

    return AsianOptionInputParameters(time, fwds, strike, yte, yts, sds, df);
}

AsianOptionInputParameters AsianOptionTest::getAsianInputsDuringAveraging()
{
    vector<double> time, fwds, sds;
    double strike, yte, yts, df;

    double spot = 21.13;
    strike = 22.0;
    double rfr = 0.065, yield = 0.025;
    double vol = 0.25;
    int i;
    for (i = -5; i < 15; ++i)
    {
        double yearsAhead = (double)i / 365.0;
        time.push_back(yearsAhead);
        fwds.push_back(spot); // rate set values
        if (yearsAhead <= 0)
        {
            sds.push_back(0); // rate set values
        }
        else
            sds.push_back(vol * sqrt(yearsAhead));
    }

    yte = (double)i / 365.0;
    yts = (double)(i + 2) / 365.0;
    df = exp(-rfr * yts);
    return AsianOptionInputParameters(time, fwds, strike, yte, yts, sds, df);
}

AsianOptionInputParameters AsianOptionTest::getAsianInputsJustBeforeSettlementWithHighAverage()
{

    vector<double> time, fwds, sds;
    double strike, yte, yts, df;

    double spot = 21.13;
    strike = 22.0;
    double rfr = 0.065, yield = 0.025;
    double vol = 0.25;
    int i;
    for (i = -18; i < 3; ++i)
    {
        double yearsAhead = (double)i / 365.0;
        time.push_back(yearsAhead);
        if (yearsAhead <= 0)
        {
            sds.push_back(0);
            fwds.push_back(spot * 1.2);
        }
        else
        {
            sds.push_back(vol * sqrt(yearsAhead));
            fwds.push_back(spot);
        }
    }
    yte = (double)i / 365.0;
    yts = (double)(i + 2) / 365.0;
    df = exp(-rfr * yts);
    return AsianOptionInputParameters(time, fwds, strike, yte, yts, sds, df);
}

AsianOptionInputParameters AsianOptionTest::getAsianInputsJustBeforeSettlementWithLowAverage()
{
    vector<double> time, fwds, sds;
    double strike, yte, yts, df;

    double spot = 21.13;
    strike = 22.0;
    double rfr = 0.065, yield = 0.025;
    double vol = 0.25;
    int i;
    for (i = -18; i < 3; ++i)
    {
        double yearsAhead = (double)i / 365.0;
        time.push_back(yearsAhead);
        if (yearsAhead <= 0)
        {
            sds.push_back(0);
            fwds.push_back(spot * 0.8);
        }
        else
        {
            sds.push_back(vol * sqrt(yearsAhead));
            fwds.push_back(spot);
        }
    }
    yte = (double)i / 365.0;
    yts = (double)(i + 2) / 365.0;
    df = exp(-rfr * yts);
    return AsianOptionInputParameters(time, fwds, strike, yte, yts, sds, df);
}
