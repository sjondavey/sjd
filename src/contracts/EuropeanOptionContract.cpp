#include <sjd/contracts/EuropeanOptionContract.h>

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

    EuropeanOption::EuropeanOption(EuropeanOption& optionInput)
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
        setMaturityDate(optionMaturity);
        setSettlementDate(optionSettlement);
        setStrike(X);
        setVolume(N);
        setPremiumPaymentDate(pDate);
        setPremiumAmount(pAmount);
        setBuySell(bs);
        setType(pc);
    }

    std::unique_ptr<Contract> EuropeanOption::clone()
    {
        return std::make_unique<EuropeanOption>(*this);
    }

    void EuropeanOption::setType(PutCall pc)
    {
        SingleOptionContract::setType(pc);
        if (getType() == PutCall::PUT)
        {
            option = std::make_shared<Black76Put>(UncheckedFormulaInputs{}, 0, 0, 0, 0);
        }
        else
        {
            option = std::make_shared<Black76Call>(UncheckedFormulaInputs{}, 0, 0, 0, 0);
        }
    }

    double EuropeanOption::getOptionValue(std::shared_ptr<RatesEnvironment> re)
    {
        Date valueDate = re->getAnchorDate();
        if (valueDate > getSettlementDate())
        {
            return 0;
        }
        double premium = 0;
        double F = (re->getFRS())->getForward(getSettlementDate());
        double df = (re->getDRS()->getDiscountFactor(getSettlementDate()));
        if (valueDate < getMaturityDate())
        {
            double sd = (re->getVRS())->getStandardDeviation(getMaturityDate(), getStrike(), F);
            option->setParameters(F, strike, sd, df);
            premium += option->getPremium();
            return premium * getVolume();
        }
        return option->getPremiumAfterMaturity(F, df) * volume;
    }

}
