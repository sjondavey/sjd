#include <sjd/contracts/EuropeanOptionContract.h>


using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd {


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
        constructorCommon(optionMaturity, 
                          optionSettlement, 
                          strike, 
                          type, 
                          volume, 
                          premiumPaymentDate, 
                          premiumAmount, 
                          buySellInput);
    }
        
    EuropeanOption::EuropeanOption(EuropeanOption &optionInput)
    {
        constructorCommon(optionInput.maturityDate, 
                          optionInput.settlementDate, 
                          optionInput.strike, 
                          optionInput.type, 
                          optionInput.volume, 
                          optionInput.premiumPaymentDate, 
                          optionInput.premiumAmount, 
                          optionInput.buySell);        
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
      errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("EuropeanOption"));
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
        return std::unique_ptr<Contract>(new EuropeanOption(*this));
    }

    void EuropeanOption::setType(PutCall pc)
    {
        SingleOptionContract::setType(pc);
        if (getType() == PUT) 
        {
            option = boost::shared_ptr<Black76Option>(new Black76Put(0, 0, 0, 0));
        }
        else 
        {
            option = boost::shared_ptr<Black76Option>(new Black76Call(0, 0, 0, 0));
        }
    }

   double EuropeanOption::getOptionValue(boost::shared_ptr<RatesEnvironment> re)
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
            option->setParameters(F,strike,sd,df);    
            premium += option->getPremium();
            return premium * getVolume();
        }            
        return option->getPremiumAfterMaturity(F, df) * volume;
    }

}


