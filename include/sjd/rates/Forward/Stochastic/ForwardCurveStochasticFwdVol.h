#pragma once

#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>
#include <sjd/rates/Forward/ForwardCurve.h>
#include <sjd/rates/Forward/ForwardRateSourceStochastic.h>

namespace sjd
{

    /*======================================================================================
    ForwardCurveStochasticFwdVol

     A curve containing date / rate inputs and a way to interpolate between these points.

     Note: If the first observation date is greater than the anchor date then, we will use
           the first observation date for all points between the two if
           allowExtrapolation == false
     =======================================================================================*/
    class ForwardCurveStochasticFwdVol : public ForwardRateSourceStochastic
    {
    public:
        ForwardCurveStochasticFwdVol() {};

        ForwardCurveStochasticFwdVol(QuantLib::Date anchorDate,
                                     std::vector<QuantLib::Date> observationDates,
                                     std::vector<double> forwards,
                                     ArrayInterpolatorType type,
                                     bool allowExtrapolation,
                                     std::shared_ptr<VolatilityRateSource> vrs);

        virtual ~ForwardCurveStochasticFwdVol();

        void setParameters(QuantLib::Date anchorDate,
                           std::vector<QuantLib::Date> observationDates,
                           std::vector<double> forwards,
                           ArrayInterpolatorType type,
                           bool allowExtrapolation,
                           std::shared_ptr<VolatilityRateSource> vrs);

        virtual bool isOK();

        virtual double getForward(QuantLib::Date toDate) const;
        using ForwardRateSource::getForward;

        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate, double normalRV);
        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate,
                                                               double normalRV,
                                                               std::vector<QuantLib::Date> rateSetDates,
                                                               std::vector<double>& rateSetsFromAnchorToDate);

        virtual void setForwardValuationDates(std::set<QuantLib::Date> dates);
        virtual std::vector<std::shared_ptr<ForwardRateSource>> rollForward();
        virtual std::vector<std::shared_ptr<ForwardRateSource>> rollForward(
            std::vector<QuantLib::Date> rateSetDates, std::vector<double>& rateSetsFromAnchorToFinalRollDate);

    protected:
        ForwardCurve quotes;
    };

}
