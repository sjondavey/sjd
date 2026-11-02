#include <sjd/contracts/Portfolio/RegCap/RegCapUnderlyingFactors.h>

#include <boost/algorithm/string.hpp>

using namespace std;

namespace sjd
{

    std::shared_ptr<RegCapUnderlyingFactors> generateRegCapFactors(std::string factorDescription)
    {
        std::shared_ptr<RegCapUnderlyingFactors> factors;
        boost::to_lower(factorDescription);

        if (factorDescription.compare("ir") == 0)
            factors = std::shared_ptr<sjd::RegCapUnderlyingFactors>(new sjd::InterestRatesRegCapFactors());
        else if (factorDescription.compare("fx") == 0)
            factors = std::shared_ptr<sjd::RegCapUnderlyingFactors>(new sjd::FXRegCapFactors());
        else if (factorDescription.compare("equity") == 0)
            factors = std::shared_ptr<sjd::RegCapUnderlyingFactors>(new sjd::EquitiesRegCapFactors());
        else if (factorDescription.compare("precious") == 0)
            factors = std::shared_ptr<sjd::RegCapUnderlyingFactors>(new sjd::PreciousMetalRegCapFactors());
        else if (factorDescription.compare("commodity") == 0)
            factors = std::shared_ptr<sjd::RegCapUnderlyingFactors>(new sjd::CommodityRegCapFactors());

        return factors;
    };

    /*======================================================================================
    InterestRatesRegCapFactors

     =======================================================================================*/
    double InterestRatesRegCapFactors::getFactor(double time)
    {
        if (time <= 1)
            return 0;
        else if (time <= 5)
            return 0.005;
        else
            return 0.015;
    };

    /*======================================================================================
    FXRegCapFactors

     =======================================================================================*/
    double FXRegCapFactors::getFactor(double time)
    {
        if (time <= 1)
            return 0.01;
        else if (time <= 5)
            return 0.05;
        else
            return 0.075;
    };

    /*======================================================================================
    EquitiesRegCapFactors

     =======================================================================================*/
    double EquitiesRegCapFactors::getFactor(double time)
    {
        if (time <= 1)
            return 0.06;
        else if (time <= 5)
            return 0.08;
        else
            return 0.10;
    };

    /*======================================================================================
    PreciousMetalRegCapFactors

     =======================================================================================*/
    double PreciousMetalRegCapFactors::getFactor(double time)
    {
        if (time <= 1)
            return 0.07;
        else if (time <= 5)
            return 0.07;
        else
            return 0.08;
    };

    /*======================================================================================
    CommodityRegCapFactors

     =======================================================================================*/
    double CommodityRegCapFactors::getFactor(double time)
    {
        if (time <= 1)
            return 0.1;
        else if (time <= 5)
            return 0.12;
        else
            return 0.15;
    };
}
