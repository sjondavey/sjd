#ifndef REGCAPCREDITPARAMTERS_INCLUDED
#define REGCAPCREDITPARAMTERS_INCLUDED
#pragma once

namespace sjd
{

    /*======================================================================================
    RegCapFactors

     =======================================================================================*/
    class RegCapCreditParameters
    {
    public:
        RegCapCreditParameters(double counterpartyWeighting, double capitalCharge);
        virtual ~RegCapCreditParameters() {};
        virtual double getCounterpartyWeighting();
        virtual double getCapitalCharge();

    protected:
        double cpw, cc;
    };

}

#endif
