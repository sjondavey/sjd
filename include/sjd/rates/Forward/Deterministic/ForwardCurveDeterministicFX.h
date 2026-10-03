#ifndef SJD_FORWARDCURVEDETERMINISITCFX_INCLUDED
#define SJD_FORWARDCURVEDETERMINISITCFX_INCLUDED

#include <sjd/rates/Forward/ForwardCurve.h>
#include <sjd/rates/Forward/ForwardCurveFX.h>
#include <sjd/rates/Forward/ForwardRateSourceDeterministic.h>
#include <sjd/rates/Forward/UsesInterestRateDifferential.h>

namespace sjd 
{

   /*======================================================================================
   ForwardCurveStochasticFX 
    
    A curve containing date / rate inputs and a way to interpolate between these points.
    
    Note: If the first observation date is greater than the anchor date then, we will use
          the first observation date for all points between the two if 
          allowExtrapolation == false
    =======================================================================================*/
    class ForwardCurveDeterministicFX : public ForwardRateSourceDeterministic, 
                                        public UsesInterestRateDifferential
    {
    public:
        ForwardCurveDeterministicFX() {};

        ForwardCurveDeterministicFX(QuantLib::Date anchorDate, 
                       double spot,
                       QuantLib::Date spotDate,
                       std::shared_ptr<DiscountRateSource> domesticDRS,
                       std::shared_ptr<DiscountRateSource> foreignDRS);

        ForwardCurveDeterministicFX(QuantLib::Date anchorDate, 
                       double spotAtT0,
                       std::shared_ptr<DiscountRateSource> domesticDRS,
                       std::shared_ptr<DiscountRateSource> foreignDRS);

        virtual ~ForwardCurveDeterministicFX();

        void setParameters(QuantLib::Date anchorDate, 
                           double spotAtT0,
                           std::shared_ptr<DiscountRateSource> domesticDRS,
                           std::shared_ptr<DiscountRateSource> foreignDRS);

        virtual void setSpotAtT0(double spotAtT0);
        virtual double getSpotAtT0();

        virtual void setRateSources(std::shared_ptr<DiscountRateSource> domesticDRS,
                                    std::shared_ptr<DiscountRateSource> foreignDRS);

        virtual bool isOK();

        virtual double getForward(QuantLib::Date toDate) const;
        using ForwardRateSource::getForward;


        virtual std::shared_ptr<ForwardRateSource> parallelBump(double spread);
        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate);

    protected:
        ForwardCurve quotes;
    };

}

#endif