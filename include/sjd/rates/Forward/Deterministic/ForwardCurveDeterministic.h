#ifndef SJD_FORWARDCURVERATESOURCE_INCLUDED
#define SJD_FORWARDCURVERATESOURCE_INCLUDED

#include <sjd/rates/Forward/ForwardCurve.h>
#include <sjd/rates/Forward/ForwardRateSourceDeterministic.h>


namespace sjd 
{

   /*======================================================================================
   ForwardCurveDeterministic 
    
    A curve containing date / rate inputs and a way to interpolate between these points.
    
    Note: If the first observation date is greater than the anchor date then, we will use
          the first observation date for all points between the two if 
          allowExtrapolation == false
    =======================================================================================*/
    class ForwardCurveDeterministic : public ForwardRateSourceDeterministic
    {
    public:
        ForwardCurveDeterministic() {};

        ForwardCurveDeterministic(QuantLib::Date anchorDate, 
                                  std::vector<QuantLib::Date> observationDates, 
                                  std::vector<double> forwards, 
                                  ArrayInterpolatorType type = LINEAR,
                                  bool allowExtrapolation = false);

        virtual bool isOK();
        virtual std::string validationRoot() const { return "forward_curve"; }

        void setParameters(QuantLib::Date anchorDate, 
                           std::vector<QuantLib::Date> observationDates, 
                           std::vector<double> forwards, 
                           ArrayInterpolatorType type = LINEAR,
                           bool allowExtrapolation = false);

        virtual double getForward(QuantLib::Date toDate) const;
        using ForwardRateSource::getForward;

        std::vector<QuantLib::Date> getObservationDates();
        std::vector<double> getForwards();
        // ensure the updated forwards have the correct size before calling this method.
        void setForwards(std::vector<double> updatedForwards);

        virtual std::shared_ptr<ForwardRateSource> parallelBump(double spread);
        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate);
        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate, 
                                                                 std::vector<QuantLib::Date> rateSetDates,
                                                                 std::vector<double> &rateSetsFromAnchorToDate);

        virtual std::vector<std::shared_ptr<ForwardRateSource> > rollForward();
        virtual std::vector<std::shared_ptr<ForwardRateSource> > rollForward(std::vector<QuantLib::Date> rateSetDates,
                                                              std::vector<double> &rateSetsFromAnchorToFinalRollDate);


    protected:
        ForwardCurve quotes;
    };

}

#endif