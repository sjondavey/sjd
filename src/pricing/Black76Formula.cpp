#include <sjd/pricing/Black76Formula.h>

#include <sjd/tools/Errors.h>

using namespace std;

namespace sjd
{

    /*======================================================================================
    Black76Option
    =======================================================================================*/
    void Black76Option::assign(double forward, double strike, double standardDeviation, double discountFactor)
    {
        F = forward;
        X = strike;
        sd = standardDeviation;
        df = discountFactor;
    }

    void Black76Option::requireInputs(double forward,
                                      double strike,
                                      double standardDeviation,
                                      double discountFactor)
    {
        if (forward <= 0)
            throw ConfigurationError("Black76 forward must be positive");
        if (strike <= 0)
            throw ConfigurationError("Black76 strike must be positive");
        if (standardDeviation <= 0)
            throw ConfigurationError("Black76 standard deviation must be positive");
        if (discountFactor <= 0)
            throw ConfigurationError("Black76 discount factor must be positive");
    }

    void Black76Option::setParameters(double forward,
                                      double strike,
                                      double standardDeviation,
                                      double discountFactor)
    {
        SJD_ASSERT(forward > 0 && strike > 0 && standardDeviation > 0 && discountFactor > 0);
        assign(forward, strike, standardDeviation, discountFactor);
    }

    void Black76Option::calculateInternalOptionParameters()
    {
        d1 = (log(F / X) / sd + sd / 2.0);
        d2 = d1 - (sd);
        Nd1 = cdf(n_0_1, d1);
        Nd2 = cdf(n_0_1, d2);
    }

    void Black76Option::setForward(double forwardInput)
    {
        SJD_ASSERT(forwardInput > 0);
        F = forwardInput;
    }

    void Black76Option::setStrike(double strikeInput)
    {
        SJD_ASSERT(strikeInput > 0);
        X = strikeInput;
    }

    void Black76Option::setStandardDeviation(double standardDeviation)
    {
        SJD_ASSERT(standardDeviation > 0);
        sd = standardDeviation;
    }

    void Black76Option::setDiscountFactor(double discountFactor)
    {
        SJD_ASSERT(discountFactor > 0);
        df = discountFactor;
    }

    /*======================================================================================
    EuropeanCallBlack76
    =======================================================================================*/
    Black76Call::Black76Call(double f, double x, double sd, double df)
    {
        requireInputs(f, x, sd, df);
        assign(f, x, sd, df);
    }

    Black76Call::Black76Call(UncheckedFormulaInputs, double f, double x, double sd, double df)
    {
        assign(f, x, sd, df);
    }

    double Black76Call::getPremium()
    {
        calculateInternalOptionParameters();
        return df * (F * Nd1 - X * Nd2);
    }

    double Black76Call::getDelta()
    {
        calculateInternalOptionParameters();
        return Nd1 * df;
    }

    /*======================================================================================
     EuropeanPutBlack76
     =======================================================================================*/
    Black76Put::Black76Put(double F, double X, double sd, double df)
    {
        requireInputs(F, X, sd, df);
        assign(F, X, sd, df);
    }

    Black76Put::Black76Put(UncheckedFormulaInputs, double F, double X, double sd, double df)
    {
        assign(F, X, sd, df);
    }

    double Black76Put::getPremium()
    {
        calculateInternalOptionParameters();
        return df * (-F * (1 - Nd1) + X * (1 - Nd2));
    }

    double Black76Put::getDelta()
    {
        calculateInternalOptionParameters();
        return (Nd1 - 1.0) * df;
    }

}