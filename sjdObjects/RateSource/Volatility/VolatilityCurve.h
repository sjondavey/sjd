#ifndef SJD_OBJ_VOLATILITYCURVE
#define SJD_OBJ_VOLATILITYCURVE

#include <sjd/rates/Volatility/VolatilityCurve.h>
#include <sjd/rates/Volatility/VolatilityCurveCMA.h>
#include <sjd/rates/Volatility/VolatilitySurfaces.h>
#include <sjd/sjdObjects/RateSource/Volatility/VolatilityRateSource.h>

#include <ql/time/date.hpp>
#include <ql/time/daycounters/simpledaycounter.hpp>

using namespace std;
using namespace QuantLib;

namespace sjd 
{
    enum ArrayInterpolatorType;
}

namespace sjdObjects 
{

   /*======================================================================================
   sjdObjects::VolatilityCurve

    =======================================================================================*/
    class VolatilityCurve : public DeterministicVolatilityRateSource
    {
    public:
        VolatilityCurve(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                        Date anchorDate, 
                        vector<Date> observationDates, 
                        vector<double> volatility, 
                        DayCounter dayCounter,
                        sjd::ArrayInterpolatorType type,
                        bool allowExtrapolation,
                        bool permanent);
              

    protected:
        OH_OBJ_CTOR(VolatilityCurve, DeterministicVolatilityRateSource);
    };

    /*======================================================================================
    sjdObjects::VolatilityCurveCMA

    Calendar Month Average vol vurve
    =======================================================================================*/
    class VolatilityCurveCMA : public DeterministicVolatilityRateSource
    {
    public:
        VolatilityCurveCMA(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                           Date anchorDate, 
                           vector<Date> observationDates, 
                           vector<double> volatility, 
                           DayCounter dayCounter,
                           sjd::ArrayInterpolatorType type,
                           bool allowExtrapolation,
                           bool permanent);
              

    protected:
        OH_OBJ_CTOR(VolatilityCurveCMA, DeterministicVolatilityRateSource);
    };


}

#endif
