#pragma once

#include <boost/math/distributions/normal.hpp> // for normal_distribution

namespace sjd
{

    /**
     * Black '76 on unitless inputs: forward, strike, standard deviation, and a
     * discount factor. There is no day count and no calendar in this class.
     * The contract turns curves into these numbers before calling.
     *
     * Put and call are separate types so the formula does not branch on a flag.
     * A trading greek is another valuation of the contract with bumped inputs.
     * getDelta() is the formula delta used to build a delta volatility surface.
     *
     * The constructor throws ConfigurationError when an input is not positive.
     * setParameters() is the Monte Carlo setter: it asserts those inputs in Debug
     * and does not check them in Release. Settlement after expiry belongs to the
     * contract, not here.
     */
    struct UncheckedFormulaInputs
    {
    };

    class Black76Option
    {

    public:
        void setParameters(double forward, double strike, double standardDeviation, double discountFactor);
        // No check. Used for a sentinel Black76 that is not priced, and after the caller has already
        // validated the inputs.
        void assign(double forward, double strike, double standardDeviation, double discountFactor);

        virtual ~Black76Option() = default;

        double getForward() { return F; };
        void setForward(double forwardInput);
        double getStrike() const { return X; };
        void setStrike(double strikeInput);
        double getStandardDeviation() { return sd; };
        void setStandardDeviation(double sd);
        double getDiscountFactor() { return df; };
        void setDiscountFactor(double df);

        virtual double getPremium() = 0;
        // After maturity but before settlement
        virtual double getPremiumAfterMaturity(double rateSetRate, double discountFactor) = 0;
        virtual double getPremiumAfterSettlement() { return 0; };

        // I have had to include this for use in the delta surface rather than for
        // use in an option
        virtual double getDelta() = 0;

    protected:
        static void requireInputs(double forward,
                                  double strike,
                                  double standardDeviation,
                                  double discountFactor);

        // When changing any of the input parameters we need to recalculate d1, d2, Nd1, Nd2
        void calculateInternalOptionParameters();

        double F, sd, df, X;
        // boost N(0,1)
        boost::math::normal n_0_1;
        double d1, d2, Nd1, Nd2;
    };

    /*======================================================================================
    Black76Call: The call option
    =======================================================================================*/
    class Black76Call : public Black76Option
    {
    public:
        Black76Call(double forward, double strike, double standardDeviation, double discountFactor);
        Black76Call(UncheckedFormulaInputs,
                    double forward,
                    double strike,
                    double standardDeviation,
                    double discountFactor);
        double getPremium();
        double getPremiumAfterMaturity(double rateSetRate, double discountFactor)
        {
            return std::max(rateSetRate - X, 0.0) * discountFactor;
        };
        double getDelta();
    };

    /*======================================================================================
     EuropeanPutBlack76
     =======================================================================================*/
    class Black76Put : public Black76Option
    {
    public:
        Black76Put(double forward, double strike, double standardDeviation, double discountFactor);
        Black76Put(UncheckedFormulaInputs,
                   double forward,
                   double strike,
                   double standardDeviation,
                   double discountFactor);
        double getPremium();
        double getPremiumAfterMaturity(double rateSetRate, double discountFactor)
        {
            return std::max(X - rateSetRate, 0.0) * discountFactor;
        };
        double getDelta();
    };

}
