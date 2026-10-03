#ifndef EUROPEANFORWARDCONTRACT_INCLUDED
#define EUROPEANFORWARDCONTRACT_INCLUDED
#pragma once

#include "Contracts.h"

namespace sjd 
{

   /*======================================================================================
   ForwardContract
    Base Class for single forward contracts. This is not intended to house a collection of 
    Forwards only single contracts.

    This contract does not contain any calendar functionality so all dates must be supplied
    as "good dates" by the user.

    NB: Forwards do NOT have PREMIUM
    =======================================================================================*/
    class ForwardContract : public SingleDerivativeContract
    {
    public:
        virtual ~ForwardContract() {};

        // Implemented for European Forwards
        virtual double getValue(std::shared_ptr<RatesEnvironment> re);
        virtual double getPayoff(double forSpot);
    };


   /*======================================================================================
   EuropeanForward

    The simplest forward contract 

    This contract does not contain any calendar functionality so all dates must be supplied
    as "good dates" by the user.
    =======================================================================================*/
    class EuropeanForward : public ForwardContract
    {
    public:
        // If the forward is going to be cash settled against an obsetvable rate then the rateSetDate is the 
        // date on which the underlying index is observed and the cash exchanges hands on the settlement
        EuropeanForward(QuantLib::Date maturityDate, QuantLib::Date settlementDate, double strike, double volume, BuySell buySell);
        // If the forward is an exchange of the asset for cash we do not need a rate std::set date
        EuropeanForward(QuantLib::Date settlementDate, double strike, double volume, BuySell buySell);

        EuropeanForward(EuropeanForward &forward);

        std::unique_ptr<Contract> clone() override;

        virtual ~EuropeanForward() {};
        virtual std::string validationRoot() const { return "european_forward"; }

    protected:
        void constructorCommon(QuantLib::Date rateSetDate, QuantLib::Date settlementDate, double strike, double volume, BuySell buySell);
    };

}

#endif

