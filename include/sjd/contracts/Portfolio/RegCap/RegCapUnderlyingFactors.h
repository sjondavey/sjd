#pragma once

#include <boost/algorithm/string.hpp>
#include <memory>
/*======================================================================================
Factors for add-ons: From Annex 4: Treatment of Counterparty Credit Risk

            Int Rates   FX&XAU  Equities    Precious    Other
                                            Metals      Commodities
                                            (ex xau)
    <= 1y       0.0%    1.0%    6.0%        7.0%        10.0%
    (1y,5y]     0.5%    5.0%    8.0%        7.0%        12.0%
    >=5y        1.5%    7.5%    10.0%       8.0%        15.0%

=======================================================================================*/

namespace sjd
{

    /*======================================================================================
    RegCapFactors

     =======================================================================================*/
    class RegCapUnderlyingFactors
    {
    public:
        virtual ~RegCapUnderlyingFactors() {};
        virtual double getFactor(double time) = 0;
    };

    /*======================================================================================
    InterestRatesRegCapFactors

     =======================================================================================*/
    class InterestRatesRegCapFactors : public RegCapUnderlyingFactors
    {
    public:
        virtual ~InterestRatesRegCapFactors() {};
        double getFactor(double time);
    };

    /*======================================================================================
    FXRegCapFactors

     =======================================================================================*/
    class FXRegCapFactors : public RegCapUnderlyingFactors
    {
    public:
        virtual ~FXRegCapFactors() {};
        double getFactor(double time);
    };

    /*======================================================================================
    EquitiesRegCapFactors

     =======================================================================================*/
    class EquitiesRegCapFactors : public RegCapUnderlyingFactors
    {
    public:
        virtual ~EquitiesRegCapFactors() {};
        double getFactor(double time);
    };

    /*======================================================================================
    PreciousMetalRegCapFactors

     // For gold, please use the FX factors
     =======================================================================================*/
    class PreciousMetalRegCapFactors : public RegCapUnderlyingFactors
    {
    public:
        virtual ~PreciousMetalRegCapFactors() {};
        double getFactor(double time);
    };

    /*======================================================================================
    CommodityRegCapFactors

     =======================================================================================*/
    class CommodityRegCapFactors : public RegCapUnderlyingFactors
    {
    public:
        virtual ~CommodityRegCapFactors() {};
        double getFactor(double time);
    };

    std::shared_ptr<RegCapUnderlyingFactors> generateRegCapFactors(std::string factorDescription);
}
