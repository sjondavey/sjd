#pragma once

#include "Contracts.h"
#include <sjd/maths/maths.h>

namespace sjd
{

    /*======================================================================================
    FixedCashflow

     We need to have a Contract for each individual cashflow. This is becuase when trying to
     calcuate regulatory capital we will be grouping contracts by their settlement date and
     this would be difficult if we booked multiple cashflows in a SingleInstrumentContract
     =======================================================================================*/
    class FixedCashflow : public SingleInstrumentContract
    {
    public:
        FixedCashflow(QuantLib::Date cashFlowDate, double volume, BuySell buySellAmount);

        FixedCashflow(const FixedCashflow& cf);

        std::unique_ptr<Contract> clone() const override;

        void commonConstructor(QuantLib::Date cashFlowDate, double volume, BuySell buySellAmount);

        virtual ~FixedCashflow() {};

        void runChecks() override;
        void runChecks(RatesEnvironment& re) override;

        virtual double getValue(const std::shared_ptr<RatesEnvironment>& re) const;
        virtual double getPayoff(double forSpot) const;
    };

}
