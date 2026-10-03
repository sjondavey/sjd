#ifndef SJD_FORWARDCURVESTOCHASTICFX_INCLUDED
#define SJD_FORWARDCURVESTOCHASTICFX_INCLUDED

#include <sjd/rates/Forward/ForwardCurve.h>
#include <sjd/rates/Forward/ForwardCurveFX.h>
#include <sjd/rates/Forward/ForwardRateSourceStochastic.h>
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
    class ForwardCurveStochasticFX : public ForwardRateSourceStochastic, 
                                     public UsesInterestRateDifferential
    {
    public:
        ForwardCurveStochasticFX() {};

        ForwardCurveStochasticFX(QuantLib::Date anchorDate, 
                                 double spot,
                                 QuantLib::Date spotDate,
                                 std::shared_ptr<DiscountRateSource> domesticDRS,
                                 std::shared_ptr<DiscountRateSource> foreignDRS,
                                 std::shared_ptr<VolatilityRateSource> vrs);

        ForwardCurveStochasticFX(QuantLib::Date anchorDate, 
                                 double spotAtT0,
                                 std::shared_ptr<DiscountRateSource> domesticDRS,
                                 std::shared_ptr<DiscountRateSource> foreignDRS,
                                 std::shared_ptr<VolatilityRateSource> vrs);

        virtual ~ForwardCurveStochasticFX();

        virtual bool isOK();

        void setParameters(QuantLib::Date anchorDate, 
                           double spot,
                           std::shared_ptr<DiscountRateSource> domesticDRS,
                           std::shared_ptr<DiscountRateSource> foreignDRS,
                           std::shared_ptr<VolatilityRateSource> vrs);

        virtual void setSpotAtT0(double spotAtT0);
        virtual double getSpotAtT0();

        virtual void setRateSources(std::shared_ptr<DiscountRateSource> domesticDRS,
                                    std::shared_ptr<DiscountRateSource> foreignDRS);


        virtual double getForward(QuantLib::Date toDate) const;
        using ForwardRateSource::getForward;

        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate, double normalRV);
        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate, 
                                                                 double normalRV,
                                                                 std::vector<QuantLib::Date> rateSetDates,
                                                                 std::vector<double> &rateSetsFromAnchorToDate);

        virtual void setForwardValuationDates(std::set<QuantLib::Date> dates);
        virtual std::vector<std::shared_ptr<ForwardRateSource> > rollForward();
        virtual std::vector<std::shared_ptr<ForwardRateSource> > rollForward(std::vector<QuantLib::Date> rateSetDates,
                                                              std::vector<double> &rateSetsFromAnchorToFinalRollDate);


    protected:
        ForwardCurve quotes;
    };

}

#endif