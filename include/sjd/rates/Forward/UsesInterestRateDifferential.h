#ifndef SJD_USESINTERESTRATEDIFFERENTIAL_INCLUDED
#define SJD_USESINTERESTRATEDIFFERENTIAL_INCLUDED

#include <sjd/rates/Discount/DiscountRateSource.h>

namespace sjd 
{

    /*======================================================================================
    UsesInterestRateDifferential
    
    When using FX style curves to generate forward paths, it is helpful to have an interface
    to update the interest rate differentials. This allows the interest rate differentials
    to roll in their own way and lets the FX forward curve stay consistent with these
    =======================================================================================*/
    class UsesInterestRateDifferential
    {
    public:
        // MUST have at least on virtual (as opposed to pure virtual) function to be 
        // polymorphic i.e. so that boost::dynamic_pointer_cast from a ForwardRateSource
        // to UsesInterestRateDifferential returns true if the frs implements this
        // interface
        virtual ~UsesInterestRateDifferential() {};

        virtual void setSpotAtT0(double spotAtT0) = 0;
        virtual double getSpotAtT0() = 0;

        virtual void setRateSources(boost::shared_ptr<DiscountRateSource> domesticDRS,
                                    boost::shared_ptr<DiscountRateSource> foreignDRS) = 0;
    };

}

#endif