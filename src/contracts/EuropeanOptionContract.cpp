#include <sjd/contracts/EuropeanOptionContract.h>
#include <sjd/pricing/Black76Formula.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    EuropeanOption

     The simplest Black-Scholes Option Contract used to price Black options on futures or
     Black Scholes options.
     =======================================================================================*/
    EuropeanOption::EuropeanOption(Date optionMaturity,
                                   Date optionSettlement,
                                   double strike,
                                   PutCall type,
                                   double volume,
                                   Date premiumPaymentDate,
                                   double premiumAmount,
                                   BuySell buySellInput)
    {
        constructorCommon(optionMaturity, optionSettlement, strike, type, volume, premiumPaymentDate,
                          premiumAmount, buySellInput);
    }

    EuropeanOption::EuropeanOption(const EuropeanOption& optionInput)
    {
        constructorCommon(optionInput.maturityDate, optionInput.settlementDate, optionInput.strike,
                          optionInput.type, optionInput.volume, optionInput.premiumPaymentDate,
                          optionInput.premiumAmount, optionInput.buySell);
    }

    void EuropeanOption::constructorCommon(Date optionMaturity,
                                           Date optionSettlement,
                                           double X,
                                           PutCall pc,
                                           double N,
                                           Date pDate,
                                           double pAmount,
                                           BuySell bs)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("EuropeanOption");
        initialise(optionMaturity, optionSettlement, N, bs, pDate, pAmount);
        initialiseStrike(X);
        initialiseType(pc);
    }

    std::unique_ptr<Contract> EuropeanOption::clone() const
    {
        return std::make_unique<EuropeanOption>(*this);
    }

    double EuropeanOption::getOptionValue(const std::shared_ptr<RatesEnvironment>& re) const
    {
        Date valueDate = re->getAnchorDate();
        if (valueDate > getSettlementDate())
        {
            return 0;
        }
        double F = (re->getFRS())->getForward(getSettlementDate());
        double df = (re->getDRS()->getDiscountFactor(getSettlementDate()));
        if (valueDate < getMaturityDate())
        {
            double sd = (re->getVRS())->getStandardDeviation(getMaturityDate(), getStrike(), F);
            if (type == PutCall::PUT)
            {
                Black76Put put(UncheckedFormulaInputs{}, F, strike, sd, df);
                return put.getPremium() * getVolume();
            }
            Black76Call call(UncheckedFormulaInputs{}, F, strike, sd, df);
            return call.getPremium() * getVolume();
        }
        if (type == PutCall::PUT)
        {
            Black76Put put(UncheckedFormulaInputs{}, 0, strike, 0, df);
            return put.getPremiumAfterMaturity(F, df) * volume;
        }
        Black76Call call(UncheckedFormulaInputs{}, 0, strike, 0, df);
        return call.getPremiumAfterMaturity(F, df) * volume;
    }

}
