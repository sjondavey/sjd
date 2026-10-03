#ifndef asianoption_test_maths
#define asianoption_test_maths

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/math/special_functions/fpclassify.hpp> // boost::math::isnan
#include <support/TestingParameters.h>
#include <sjd/pricing/AsianOptionFormula.h>

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

    AsianOptionInputParameters(std::vector<double> forwardRates,         // "Live" forwards
                               std::vector<double> fixedRates,
                               double strike, 
                               std::vector<double> standardDeviation, 
                               double discountFactorToSettlement);

    std::vector<double> fwds, sds, fixes;
    double strike, df; 
    size_t N, M;


};

class AsianOptionTest {
  public:
    static void testErrorMessages();
    static void testErrorMessagesWithSpecificData(AsianOptionInputParameters inputParamters);

    static void testValulationBeforeAveraging();
    static void testValulationDuringAveraging();
    static void testValulationJustBeforeSettlementWithHighAverage();
    static void testValulationJustBeforeSettlementWithLowAverage();

    static boost::unit_test_framework::test_suite* suite();

private:
    static AsianOptionInputParameters getAsianInputsBeforeAveragingStarts();
    static AsianOptionInputParameters getAsianInputsDuringAveraging(); // soon after averaging starts
    static AsianOptionInputParameters getAsianInputsJustBeforeSettlementWithHighAverage(); // relative to strike
    static AsianOptionInputParameters getAsianInputsJustBeforeSettlementWithLowAverage(); // realtive to strike
};

#endif
