#include <sjd/pricing/AsianOptionFormula.h>

#include <sjd/tools/Errors.h>

using namespace std;

namespace sjd
{

    /*======================================================================================
    AverageRateOption
     =======================================================================================*/
    void AverageRateOption::store(vector<double> forwardRatesInput,
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

    std::string AverageRateOption::inputProblem() const
    {
        if (standardDeviations.size() != forwards.size())
            return "Standard deviation and forward inputs do not have the same length";
        if (M > N)
            return "The number of rate set points exceeds the total number of averaging points";
        if (M == N)
            return "All forwards have rate set, this is not an option anymore";
        for (size_t i = 0; i < forwards.size(); ++i)
        {
            if (forwards[i] <= 0)
                return "All rate sets or forwards must be positive";
            if (standardDeviations[i] <= 0)
                return "All live forwards must have positive standard deviations";
        }
        if (strike <= 0)
            return "Strike must be positive";
        if (dfToSettlement <= 0)
            return "Discount factor must be positive";
        return {};
    }

    void AverageRateOption::setParameters(vector<double> forwardRatesInput,
                                          vector<double> fixedRatesInputs,
                                          double strikeInput,
                                          vector<double> standardDeviationsInput,
                                          double dfToSettlementInput)
    {
        store(forwardRatesInput, fixedRatesInputs, strikeInput, standardDeviationsInput, dfToSettlementInput);
        SJD_ASSERT(inputProblem().empty());
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

    void AverageRateOption::callWhenSettingAnyInternalParameter()
    {
        amountByWhichAverageExceedsStrike = 0;
        effectiveForward = 0;
        effectiveStrike = 0;
        effectiveStandardDeviation = 0;
        option->assign(0, 0, 0, 0);
        internalParamtersSet = false;
    }

    void AverageRateOption::calculateInternalOptionParameters()
    {
        if (!internalParamtersSet)
        {
            getEquivalentBlack76ParamtersFromVectorInputs();
            if (amountByWhichAverageExceedsStrike > 0)
                option->assign(0, 0, 0, 0);
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
        option = new Black76Call(UncheckedFormulaInputs{}, 0, 0, 0, 0);
        store(forwardRates, fixedRates, strike, standardDeviation, discountFactorToSettlement);
        const std::string problem = inputProblem();
        if (!problem.empty())
            throw ConfigurationError(problem);
    }

    AverageRateCall::AverageRateCall(UncheckedFormulaInputs,
                                     vector<double> forwardRates,
                                     vector<double> fixedRates,
                                     double strike,
                                     vector<double> standardDeviation,
                                     double discountFactorToSettlement)
    {
        option = new Black76Call(UncheckedFormulaInputs{}, 0, 0, 0, 0);
        store(forwardRates, fixedRates, strike, standardDeviation, discountFactorToSettlement);
    }

    AverageRateCall::~AverageRateCall()
    {
        delete option;
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
        option = new Black76Put(UncheckedFormulaInputs{}, 0, 0, 0, 0);
        store(forwardRates, fixedRates, strike, standardDeviation, discountFactorToSettlement);
        const std::string problem = inputProblem();
        if (!problem.empty())
            throw ConfigurationError(problem);
    }

    AverageRatePut::AverageRatePut(UncheckedFormulaInputs,
                                   vector<double> forwardRates,
                                   vector<double> fixedRates,
                                   double strike,
                                   vector<double> standardDeviation,
                                   double discountFactorToSettlement)
    {
        option = new Black76Put(UncheckedFormulaInputs{}, 0, 0, 0, 0);
        store(forwardRates, fixedRates, strike, standardDeviation, discountFactorToSettlement);
    }

    AverageRatePut::~AverageRatePut()
    {
        delete option;
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