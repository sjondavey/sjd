#include <sjd/contracts/Cashflow.h>


using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{

   /*======================================================================================
   FixedCashflow

    =======================================================================================*/
    void FixedCashflow::commonConstructor(Date cashFlowDate, 
                                          double amount, 
                                          BuySell bs)
    {
      errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("FixedCashflow"));
        setMaturityDate(cashFlowDate);
        setSettlementDate(cashFlowDate);
        setVolume(abs(amount));
        setBuySell(bs);
        setPremiumPaymentDate(cashFlowDate);
        setPremiumAmount(0);
    }


    FixedCashflow::FixedCashflow(Date cashFlowDate, double amount, BuySell bs)
    {
        commonConstructor(cashFlowDate, amount, bs);
    }

    FixedCashflow::FixedCashflow(FixedCashflow &cf)
    {
        commonConstructor(cf.getMaturityDate(), cf.getVolume(), cf.getBuySell());
    }

    std::unique_ptr<Contract> FixedCashflow::clone()
    {
        return std::unique_ptr<Contract>(new FixedCashflow(*this));
    }

    void FixedCashflow::validate()
    {
        SingleInstrumentContract::validate();
        if (getMaturityDate() != getSettlementDate())
      {
            recordIssue("CASHFLOW_MATURITY_NOT_SETTLEMENT", "Maturity Date must equal Settlement Date", {"maturity_date"});
      }
    }

    void FixedCashflow::validate(std::shared_ptr<RatesEnvironment> re)
    {
        SingleInstrumentContract::validate(re);
    }

    void FixedCashflow::setMaturityDate(Date maturityDate)
    {
        SingleInstrumentContract::setMaturityDate(maturityDate);
        SingleInstrumentContract::setSettlementDate(maturityDate);
    }

    void FixedCashflow::setSettlementDate(Date settlementDate)
    {
        SingleInstrumentContract::setMaturityDate(settlementDate);
        SingleInstrumentContract::setSettlementDate(settlementDate);
    }

    void FixedCashflow::setPremiumAmount(double premium)
    {
        premiumAmount = 0;
    }

    double FixedCashflow::getValue(std::shared_ptr<RatesEnvironment> re)
    {
        Date valueDate = re->getAnchorDate();
        if (valueDate > settlementDate)
        {
            return 0;
        }
        double pv = 0;
        double pvCashFlows = volume * (re->getDRS())->getDiscountFactor(settlementDate);
        return buySellAsDouble * (pvCashFlows);
    }

    double FixedCashflow::getPayoff(double forSpot)
    {
        return buySellAsDouble * volume;
    }

}

