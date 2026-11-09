#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sjd/pricing/AsianOptionFormula.h>
#include <sjd/tools/Errors.h>
#include <support/TestingParameters.h>

/*======================================================================================
AsianOptionInputParameters
A simple class to group inputs for Asian Option. This makes testing a lot neater
=======================================================================================*/
class AsianOptionInputParameters
{
public:
    AsianOptionInputParameters(std::vector<double> yearFractionsToRateSets,
                               std::vector<double> fixingAndForwardRates,
                               double strike,
                               double yearsToExpiry,
                               double yearsToSettlement,
                               std::vector<double> standardDeviation,
                               double discountFactorToSettlement);

    AsianOptionInputParameters(std::vector<double> forwardRates, // "Live" forwards
                               std::vector<double> fixedRates,
                               double strike,
                               std::vector<double> standardDeviation,
                               double discountFactorToSettlement);

    std::vector<double> fwds, sds, fixes;
    double strike, df;
    size_t N, M;
};

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
                                                       double /*yte*/,
                                                       double /*yts*/,
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

BOOST_AUTO_TEST_SUITE(AsianOptionTest)

void testErrorMessagesWithSpecificData(AsianOptionInputParameters ip);
AsianOptionInputParameters getAsianInputsBeforeAveragingStarts();
AsianOptionInputParameters getAsianInputsDuringAveraging();
AsianOptionInputParameters getAsianInputsJustBeforeSettlementWithHighAverage();
AsianOptionInputParameters getAsianInputsJustBeforeSettlementWithLowAverage();

BOOST_AUTO_TEST_CASE(testErrorMessages)
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

void testErrorMessagesWithSpecificData(AsianOptionInputParameters ip)
{

    AverageRateCall call(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    AverageRatePut put(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);

    // A mismatched forward vector is a programming error on the setter used by valuation.
    vector<double> incorrectFwds = ip.fwds;
    incorrectFwds.pop_back();
#ifndef NDEBUG
    BOOST_CHECK_THROW(put.setParameters(incorrectFwds, ip.fixes, ip.strike, ip.sds, ip.df), sjd::Error);
#endif
    vector<double> incorrectSD = ip.sds;
    incorrectSD.pop_back();
    put.setParameters(incorrectFwds, ip.fixes, ip.strike, incorrectSD, ip.df);
    put.setParameters(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
}
BOOST_AUTO_TEST_CASE(testValulationBeforeAveraging)
{
    BOOST_TEST_MESSAGE("Testing Asian Option Valuation before averaging");
    AsianOptionInputParameters ip = getAsianInputsBeforeAveragingStarts();
    AverageRateCall call(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    AverageRatePut put(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);

    // Manual replication
    double effectiveStrike = ip.strike;
    double effectiveForward = 0;
    for (size_t i = 0; i < ip.N; ++i)
    {
        effectiveForward += ip.fwds[i];
    }
    effectiveForward /= ip.N;

    double NSquared = static_cast<double>(ip.N) * static_cast<double>(ip.N);
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

    BOOST_CHECK_SMALL(call.getPremium() - equivalentCall.getPremium(), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(put.getPremium() - equivalentPut.getPremium(), EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testValulationDuringAveraging)
{
    BOOST_TEST_MESSAGE("Testing Asian Option Valuation during averaging");
    AsianOptionInputParameters ip = getAsianInputsDuringAveraging();
    AverageRateCall call(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    AverageRatePut put(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);

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

    double NSquared = static_cast<double>(ip.N) * static_cast<double>(ip.N);
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

    BOOST_CHECK_SMALL(call.getPremium() - equivalentCall.getPremium(), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(put.getPremium() - equivalentPut.getPremium(), EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testValulationJustBeforeSettlementWithHighAverage)
{
    BOOST_TEST_MESSAGE("Testing Asian Option Valuation during averaging with high average");

    AsianOptionInputParameters ip = getAsianInputsJustBeforeSettlementWithHighAverage();
    AverageRateCall call(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    AverageRatePut put(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);

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

    BOOST_CHECK_SMALL(call.getPremium() - equivalentCallPremium, EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(put.getPremium() - equivalentPutPremium, EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testValulationJustBeforeSettlementWithLowAverage)
{
    BOOST_TEST_MESSAGE("Testing Asian Option Valuation during averaging with low average");
    AsianOptionInputParameters ip = getAsianInputsJustBeforeSettlementWithLowAverage();
    AverageRateCall call(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);
    AverageRatePut put(ip.fwds, ip.fixes, ip.strike, ip.sds, ip.df);

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

    double NSquared = static_cast<double>(ip.N) * static_cast<double>(ip.N);
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

    BOOST_CHECK_SMALL(call.getPremium() - equivalentCall.getPremium(), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(put.getPremium() - equivalentPut.getPremium(), EPSILON_FOR_DOUBLES);
}

AsianOptionInputParameters getAsianInputsBeforeAveragingStarts()
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

AsianOptionInputParameters getAsianInputsDuringAveraging()
{
    vector<double> time, fwds, sds;
    double strike, yte, yts, df;

    double spot = 21.13;
    strike = 22.0;
    [[maybe_unused]] double rfr = 0.065;
    [[maybe_unused]] double yield = 0.025;
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

AsianOptionInputParameters getAsianInputsJustBeforeSettlementWithHighAverage()
{

    vector<double> time, fwds, sds;
    double strike, yte, yts, df;

    double spot = 21.13;
    strike = 22.0;
    [[maybe_unused]] double rfr = 0.065;
    [[maybe_unused]] double yield = 0.025;
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

AsianOptionInputParameters getAsianInputsJustBeforeSettlementWithLowAverage()
{
    vector<double> time, fwds, sds;
    double strike, yte, yts, df;

    double spot = 21.13;
    strike = 22.0;
    [[maybe_unused]] double rfr = 0.065;
    [[maybe_unused]] double yield = 0.025;
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

BOOST_AUTO_TEST_SUITE_END()
