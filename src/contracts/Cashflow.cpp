#include <sjd/contracts/Cashflow.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    FixedCashflow

     =======================================================================================*/
    void FixedCashflow::commonConstructor(Date cashFlowDate, double amount, BuySell bs)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("FixedCashflow");
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

    FixedCashflow::FixedCashflow(FixedCashflow& cf)
    {
        commonConstructor(cf.getMaturityDate(), cf.getVolume(), cf.getBuySell());
    }

    std::unique_ptr<Contract> FixedCashflow::clone()
    {
        return std::make_unique<FixedCashflow>(*this);
    }

    void FixedCashflow::validate()
    {
        SingleInstrumentContract::validate();
        if (getMaturityDate() != getSettlementDate())
        {
            recordIssue("CASHFLOW_MATURITY_NOT_SETTLEMENT", "Maturity Date must equal Settlement Date",
                        {"maturity_date"});
        }
    }

    void FixedCashflow::validate(std::shared_ptr<RatesEnvironment> re)
    {
        SingleInstrumentContract::validate(re);
    }

    void FixedCashflow::setMaturityDate(Date maturity)
    {
        SingleInstrumentContract::setMaturityDate(maturity);
        SingleInstrumentContract::setSettlementDate(maturity);
    }

    void FixedCashflow::setSettlementDate(Date settlement)
    {
        SingleInstrumentContract::setMaturityDate(settlement);
        SingleInstrumentContract::setSettlementDate(settlement);
    }

    void FixedCashflow::setPremiumAmount(double /*premium*/)
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
        [[maybe_unused]] double pv = 0;
        double pvCashFlows = volume * (re->getDRS())->getDiscountFactor(settlementDate);
        return sign(buySell) * (pvCashFlows);
    }

    double FixedCashflow::getPayoff(double /*forSpot*/)
    {
        return sign(buySell) * volume;
    }

}
