#ifndef SJD_OBJ_FIXINGCURVES
#define SJD_OBJ_FIXINGCURVES

#include <sjd/rates/Fixing/FixingCurves.h>
#include "FixingRateSource.h"

#include <ql/time/date.hpp>

using namespace std;
using namespace QuantLib;

namespace sjdObjects 
{

   /*======================================================================================
   DailyFixingCurve

    =======================================================================================*/
    class DailyFixingCurve : public FixingRateSource
    {
    public:
        DailyFixingCurve(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                         vector<Date> fixingDates, 
                         vector<double> fixingRates,
                         bool permanent);
      
        
    protected:
        OH_OBJ_CTOR(DailyFixingCurve, FixingRateSource);
    };



   /*======================================================================================
   MonthlyFixingCurve

    =======================================================================================*/
    class MonthlyFixingCurve : public FixingRateSource
    {
    public:
        MonthlyFixingCurve(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                           vector<Date> fixingDates, 
                           vector<double> fixingRates,
                           bool permanent);
              
    protected:
        OH_OBJ_CTOR(MonthlyFixingCurve, FixingRateSource);
    };


}

#endif
