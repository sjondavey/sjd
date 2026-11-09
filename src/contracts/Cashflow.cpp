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
        initialise(cashFlowDate, cashFlowDate, amount, bs, cashFlowDate, 0);
    }

    FixedCashflow::FixedCashflow(Date cashFlowDate, double amount, BuySell bs)
    {
        commonConstructor(cashFlowDate, amount, bs);
    }

    FixedCashflow::FixedCashflow(const FixedCashflow& cf)
    {
        commonConstructor(cf.getMaturityDate(), cf.getVolume(), cf.getBuySell());
    }

    std::unique_ptr<Contract> FixedCashflow::clone() const
    {
        return std::make_unique<FixedCashflow>(*this);
    }

    void FixedCashflow::runChecks()
    {
        SingleInstrumentContract::runChecks();
        if (getMaturityDate() != getSettlementDate())
        {
            recordIssue(issues::CASHFLOW_MATURITY_NOT_SETTLEMENT, "Maturity Date must equal Settlement Date",
                        {"maturity_date"});
        }
    }

    void FixedCashflow::runChecks(RatesEnvironment& re)
    {
        SingleInstrumentContract::runChecks(re);
    }

    double FixedCashflow::getValue(const std::shared_ptr<RatesEnvironment>& re) const
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

    double FixedCashflow::getPayoff(double /*forSpot*/) const
    {
        return sign(buySell) * volume;
    }

}
