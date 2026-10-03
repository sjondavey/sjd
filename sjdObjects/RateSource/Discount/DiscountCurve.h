#ifndef SJD_OBJ_DISCOUNTCURVE
#define SJD_OBJ_DISCOUNTCURVE

#include "DiscountRateSource.h"
#include <Tools/vo_GenericUnimplemented.h>
#include <sjd/rates/Discount/InterestRateConvention.h>

namespace sjdObjects 
{
        
   /*======================================================================================
   DiscountCurve

    =======================================================================================*/
    class DiscountCurve : public DeterministicDiscountRateSource 
    {
    public:
        DiscountCurve(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                      Date anchorDate, 
                      vector<Date> observationDates, 
                      vector<double> discountFactors, 
                      sjd::ArrayInterpolatorType type,
                      bool allowExtrapolation,
                      bool permanent);  
        
        virtual boost::shared_ptr<DiscountRateSource> parallelBump(
                          const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                          double spreadPercentage, 
                          boost::shared_ptr<QuantLib::DayCounter> dayCounter, 
                          Compounding compounding, 
                          Frequency frequency,
                          bool permanent);  
        
        virtual boost::shared_ptr<DiscountRateSource> rollForward(
            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
            Date toDate,
            bool permanent);

    protected:
        OH_OBJ_CTOR(DiscountCurve, DeterministicDiscountRateSource);
    };

}

#endif
