#include <sjd/pricing/AsianOptionFormula.h>

using namespace std;

namespace sjd
{

    /*======================================================================================
    AverageRateOption
     =======================================================================================*/
    void AverageRateOption::setParameters(vector<double> forwardRatesInput,
                                          vector<double> fixedRatesInputs,
                                          double strikeInput,
                                          vector<double> standardDeviationsInput,
                                          double dfToSettlementInput)
    {
        forwards = forwardRatesInput;
        fixedRates = fixedRatesInputs;
        M = fixedRates.size();
        N = M + forwards.size();
        standardDeviations = standardDeviationsInput;
        strike = strikeInput;
        dfToSettlement = dfToSettlementInput;
        callWhenSettingAnyInternalParameter();
    }

    void AverageRateOption::setParameters(size_t nInput,
                                          size_t mInput,
                                          double averageOfLiveForwards, // "Live" forwards
                                          double averageOfFixedRates,
                                          double strikeInput,
                                          double averageStandardDeviation,
                                          double discountFactorToSettlement)
    {
        forwards = vector<double>(N - M, averageOfLiveForwards);
        fixedRates = vector<double>(M, averageOfFixedRates);
        M = mInput;
        N = nInput;
        standardDeviations = vector<double>(N - M, averageStandardDeviation);
        strike = strikeInput;
        dfToSettlement = discountFactorToSettlement;
        callWhenSettingAnyInternalParameter();
    }

    bool AverageRateOption::isOK()
    {
        // Input vectors must be the same length
        if ((standardDeviations.size() != forwards.size()))
        {
            errorMessage =
                "AverageRateOption->Standard Deviation and forward inputs do not have the same length";
            return false;
        }
        if (M > N)
        {
            errorMessage = "AverageRateOption->The number of rate set points exceeds to total number of "
                           "averaging points";
            return false;
        }
        // some forwards must be live
        [[maybe_unused]] size_t numberOfAveragingPoints = forwards.size();
        if (M == N)
        {
            errorMessage = "AverageRateOption->All forwards have rate set, this is not an option anymore";
            return false;
        }
        // inputs must be positive
        for (size_t i = 0; i < forwards.size(); ++i)
        {
            if (forwards[i] <= 0)
            {
                errorMessage = "AverageRateOption->all rate sets or forwards must be positive";
                return false;
            }
            if (standardDeviations[i] <= 0)
            {
                errorMessage =
                    "AverageRateOption->all (live) forwards must have positive standard deviations";
                return false;
            }
        }
        if (strike <= 0)
        {
            errorMessage = "AverageRateOption->strike must be positive";
            return false;
        }
        if (dfToSettlement <= 0)
        {
            errorMessage = "AverageRateOption->Dsicount factor must be positive";
            return false;
        }
        // I don't check for -ve fwd fwd variances becuase I only use variance out of T0
        errorMessage = "AverageRateOption->No errors";
        return true;
    }

    void AverageRateOption::callWhenSettingAnyInternalParameter()
    {
        amountByWhichAverageExceedsStrike = 0;
        effectiveForward = 0;
        effectiveStrike = 0;
        effectiveStandardDeviation = 0;
        option->setParameters(0, 0, 0, 0);
        internalParamtersSet = false;
        errorMessage = "AverageRateOption->Error checking has not happened, please call isOK() first to "
                       "check for errors in input parameters.";
    }

    void AverageRateOption::calculateInternalOptionParameters()
    {
        if (!internalParamtersSet)
        {
            getEquivalentBlack76ParamtersFromVectorInputs();
            if (amountByWhichAverageExceedsStrike > 0)
                option->setParameters(0, 0, 0, 0);
            else
                option->setParameters(effectiveForward, effectiveStrike, effectiveStandardDeviation,
                                      dfToSettlement);
            internalParamtersSet = true;
        }
    }

    void AverageRateOption::getEquivalentBlack76ParamtersFromVectorInputs()
    {
        amountByWhichAverageExceedsStrike = 0;
        vector<double> volatilityMeasure;
        effectiveForward = 0;
        effectiveStrike = 0;
        for (size_t i = 0; i < fixedRates.size(); ++i)
        {
            effectiveStrike += fixedRates[i];
        }
        for (size_t i = 0; i < N - M; ++i)
        {
            volatilityMeasure.push_back(standardDeviations[i] * standardDeviations[i]);
            effectiveForward += forwards[i];
        }

        // During the averaging period there is a real chance that the Average to Date (i.e. the
        // sum of all the rate set rate divided by the TOTAL number of averaging points) is greater
        // Than the option's strike. If this is the case then
        //  - Puts will expire worthless: Even if the rest of the observation points are 0, the
        //      average rate will still exceed the strike and there will ne no payoff
        //  - Calls become forwards: Even if the rest of the observation points are 0, the
        //      average rate will still exceed the strike and there will be a payoff
        if (effectiveStrike > strike * N)
        { // option has ceased to be an option, it is now a forward
            amountByWhichAverageExceedsStrike =
                effectiveStrike - strike * N; // part of the "forward" has rate set
            amountByWhichAverageExceedsStrike +=
                effectiveForward; // part of the "forward" is in the future and still variable
            amountByWhichAverageExceedsStrike /= N; // Always positive!
        }
        effectiveForward /= (double)N;
        effectiveStrike /= (-(double)N);
        effectiveStrike += strike; // may be negative if average exceeds spot

        // Note I do not check fwd fwd variance becuase I only use the vol out of T0

        double NSquared = static_cast<double>(N) * static_cast<double>(N);
        effectiveStandardDeviation = 0;
        for (size_t j = 0; j < N - M; ++j)
        {
            for (size_t i = j; i < N - M; ++i)
            {
                effectiveStandardDeviation +=
                    2 * forwards[i] * forwards[j] / NSquared * exp(volatilityMeasure[j]);
            }
            effectiveStandardDeviation -= forwards[j] * forwards[j] / NSquared * exp(volatilityMeasure[j]);
        }
        effectiveStandardDeviation = log(effectiveStandardDeviation / (effectiveForward * effectiveForward));
        effectiveStandardDeviation = sqrt(effectiveStandardDeviation);
    }

    /*======================================================================================
    AverageRateCall
     =======================================================================================*/
    AverageRateCall::AverageRateCall(vector<double> forwardRates, // "Live" forwards
                                     vector<double> fixedRates,
                                     double strike,
                                     vector<double> standardDeviation,
                                     double discountFactorToSettlement)

    {
        option = new Black76Call(0, 0, 0, 0);
        setParameters(forwardRates, fixedRates, strike, standardDeviation, discountFactorToSettlement);
    };

    AverageRateCall::~AverageRateCall()
    {
        delete option;
    }

    bool AverageRateCall::isOK()
    {
        bool isOKVariable = AverageRateOption::isOK();
        errorMessage = "AverageRateCall->" + errorMessage;
        return isOKVariable;
    }

    double AverageRateCall::getPremium()
    {
        if (!internalParamtersSet)
            calculateInternalOptionParameters();
        if (effectiveStrike > 0)
            return option->getPremium();
        else
            return amountByWhichAverageExceedsStrike * dfToSettlement;
    }

    /*======================================================================================
    AverageRatePut
     =======================================================================================*/
    AverageRatePut::AverageRatePut(vector<double> forwardRates, // "Live" forwards
                                   vector<double> fixedRates,
                                   double strike,
                                   vector<double> standardDeviation,
                                   double discountFactorToSettlement)

    {
        option = new Black76Put(0, 0, 0, 0);
        setParameters(forwardRates, fixedRates, strike, standardDeviation, discountFactorToSettlement);
    };

    AverageRatePut::~AverageRatePut()
    {
        delete option;
    }

    bool AverageRatePut::isOK()
    {
        bool isOKVariable = AverageRateOption::isOK();
        errorMessage = "AverageRatePut->" + errorMessage;
        return isOKVariable;
    }

    double AverageRatePut::getPremium()
    {
        if (!internalParamtersSet)
            calculateInternalOptionParameters();
        if (effectiveStrike > 0)
            return option->getPremium();
        else
            return 0;
    }
}