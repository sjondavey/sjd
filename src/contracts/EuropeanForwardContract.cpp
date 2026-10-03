#include <sjd/contracts/EuropeanForwardContract.h>


using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd {

   /*======================================================================================
   ForwardContract

    =======================================================================================*/
    double ForwardContract::getValue(std::shared_ptr<RatesEnvironment> re)
    {
        Date valueDate = re->getAnchorDate();
        if (valueDate > settlementDate)
        {
            return 0;
        }
        double premium = 0;
        double F = (re->getFRS())->getForward(getSettlementDate());
        double df = (re->getDRS())->getDiscountFactor(getSettlementDate());

        premium = (F - strike) * df;
        return premium * volume * buySellAsDouble;
    }

    double ForwardContract::getPayoff(double forSpot)
    {
        return (forSpot - strike) * volume * buySellAsDouble;
    }

   /*======================================================================================
   EuropeanForward

    =======================================================================================*/

    void EuropeanForward::constructorCommon(Date maturity, Date settlement, double X, double N, BuySell bs)
    {
      errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("EuropeanForward"));
        setMaturityDate(maturity);
        setSettlementDate(settlement);
        setStrike(X);
        setVolume(N);
        setBuySell(bs);
        setPremiumAmount(0);
        setPremiumPaymentDate(settlement);
    }

    EuropeanForward::EuropeanForward(Date rateSetDate, Date settlementDate, double strike, double volume, BuySell buySell)
    {
        constructorCommon(rateSetDate, settlementDate, strike, volume, buySell);
    }

    EuropeanForward::EuropeanForward(Date settlementDate, double strike, double volume, BuySell buySell)
    {
        constructorCommon(settlementDate, settlementDate, strike, volume, buySell);
    }

    EuropeanForward::EuropeanForward(EuropeanForward &forward)
    {
        constructorCommon(forward.maturityDate, 
                           forward.settlementDate, 
                           forward.strike, 
                           forward.volume, 
                           forward.buySell);
    }

    std::unique_ptr<Contract> EuropeanForward::clone()
    {
        return std::unique_ptr<Contract>(new EuropeanForward(*this));
    }

}

