#include <sjd/pricing/Black76Formula.h>

using namespace std;

namespace sjd
{

    /*======================================================================================
    Black76Option
    =======================================================================================*/
    void Black76Option::setParameters(double forward,
                                      double strike,
                                      double standardDeviation,
                                      double discountFactor)
    {
        F = forward;
        X = strike;
        sd = standardDeviation;
        df = discountFactor;
        callWhenSettingAnyInternalParameter();
    }

    bool Black76Option::isOK()
    {
        if (F <= 0)
        {
            errorMessage = "Black76Option->Forward is <= 0";
            return false;
        }
        if (X <= 0)
        {
            errorMessage = "Black76Option->Strike is <= 0";
            return false;
        }
        if (sd <= 0)
        {
            errorMessage = "Black76Option->Standard Deviation is <= 0";
            return false;
        }
        if (df <= 0)
        {
            errorMessage = "Black76Option->Discount Factor is <= 0";
            return false;
        }
        errorMessage = "Black76Option->No errors";
        return true;
    }

    void Black76Option::callWhenSettingAnyInternalParameter()
    {
        errorMessage = "Black76Option->Error checking has not happened, please call isOK() first to check "
                       "for errors in input parameters.";
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
        F = forwardInput;
        callWhenSettingAnyInternalParameter();
    }

    void Black76Option::setStrike(double strikeInput)
    {
        X = strikeInput;
        callWhenSettingAnyInternalParameter();
    }

    void Black76Option::setStandardDeviation(double standardDeviation)
    {
        sd = standardDeviation;
        callWhenSettingAnyInternalParameter();
    }

    void Black76Option::setDiscountFactor(double discountFactor)
    {
        df = discountFactor;
        callWhenSettingAnyInternalParameter();
    }

    /*======================================================================================
    EuropeanCallBlack76
    =======================================================================================*/
    Black76Call::Black76Call(double f, double x, double sd, double df)
    {
        setParameters(f, x, sd, df);
    }

    bool Black76Call::isOK()
    {
        bool isOKVariable = Black76Option::isOK();
        errorMessage = "Black76Call->" + errorMessage;
        return isOKVariable;
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
        setParameters(F, X, sd, df);
    }

    bool Black76Put::isOK()
    {
        bool isOKVariable = Black76Option::isOK();
        errorMessage = "Black76Put->" + errorMessage;
        return isOKVariable;
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