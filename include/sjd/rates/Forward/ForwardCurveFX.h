#ifndef SJD_FORWARDCURVEFX_INCLUDED
#define SJD_FORWARDCURVEFX_INCLUDED

#include <sjd/rates/Forward/ForwardCurve.h>
#include <sjd/rates/Forward/ForwardRateSource.h>
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
    class ForwardCurveFX : public ForwardRateSource, public UsesInterestRateDifferential
    {
    public:
        ForwardCurveFX() {};

        ForwardCurveFX(QuantLib::Date anchorDate,
                       double spot,
                       QuantLib::Date spotDate,
                       std::shared_ptr<DiscountRateSource> domesticDRS,
                       std::shared_ptr<DiscountRateSource> foreignDRS);

        ForwardCurveFX(QuantLib::Date anchorDate,
                       double spotAtT0,
                       std::shared_ptr<DiscountRateSource> domesticDRS,
                       std::shared_ptr<DiscountRateSource> foreignDRS);

        virtual ~ForwardCurveFX();

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

    protected:
        ForwardCurve quotes;
    };

}

#endif