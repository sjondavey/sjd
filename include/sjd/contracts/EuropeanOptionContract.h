#ifndef OPTIONCONTRACT_INCLUDED
#define OPTIONCONTRACT_INCLUDED
#pragma once

#include <boost/math/special_functions/round.hpp>
#include <sjd/pricing/Black76Formula.h>

#include "Contracts.h"


namespace sjd {

   /*======================================================================================
   EuropeanOption

    The simplest Black-Scholes Option Contract used to price Black options on futures or
    Black Scholes options. This code will use the 2nd volatility dimension if the vol rate
   source is a surface.
    =======================================================================================*/
    class EuropeanOption : public SingleOptionContract 
    {
    public:
        // BuySell refers to the underlying option. Premium will have the opposite sign
        EuropeanOption(QuantLib::Date optionMaturity, 
                       QuantLib::Date optionSettlement, 
                       double strike, 
                       PutCall type, 
                       double volume,
                       QuantLib::Date premiumPaymentDate, 
                       double premiumAmount, 
                       BuySell buySell);        

        EuropeanOption(EuropeanOption &option);
        std::unique_ptr<Contract> clone() override;
        virtual ~EuropeanOption() {};

        virtual double getOptionValue(std::shared_ptr<RatesEnvironment> re);
        virtual std::string validationRoot() const { return "european_option"; }

        virtual void setType(PutCall pc);

   protected:
        void constructorCommon(QuantLib::Date optionMaturity, 
                               QuantLib::Date optionSettlement, 
                               double strike, 
                               PutCall type, 
                               double volume,
                               QuantLib::Date premiumPaymentDate, 
                               double premiumAmount, 
                               BuySell buySell);

       std::shared_ptr<Black76Option> option;
    };

}

#endif

