#pragma once

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
        // polymorphic i.e. so that std::dynamic_pointer_cast from a ForwardRateSource
        // to UsesInterestRateDifferential returns true if the frs implements this
        // interface
        virtual ~UsesInterestRateDifferential() {};

        virtual void setSpotAtT0(double spotAtT0) = 0;
        virtual double getSpotAtT0() = 0;

        virtual void setRateSources(std::shared_ptr<DiscountRateSource> domesticDRS,
                                    std::shared_ptr<DiscountRateSource> foreignDRS) = 0;
    };

}
