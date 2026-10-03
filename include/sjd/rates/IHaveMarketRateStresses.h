#ifndef SJD_IHAVEMARKETRATESTRESSES_INCLUDED
#define SJD_IHAVEMARKETRATESTRESSES_INCLUDED
#pragma once

#include "RateSource.h"

namespace sjd 
{

   /*======================================================================================
    IHaveMarketRateStresses
    
    To perform market rate stresses, Rate Sources should know how to stress themselves. 
    These rate sources will return a std::vector containing all their stresses with a std::string name 
    or description for each stress. 
    =======================================================================================*/
    class IHaveMarketRateStresses 
    {        
    public:
        virtual ~IHaveMarketRateStresses() {};

        virtual std::vector<std::pair<std::string, boost::shared_ptr<RateSource>>> getMarketRateStresses() = 0;
    };
}

#endif