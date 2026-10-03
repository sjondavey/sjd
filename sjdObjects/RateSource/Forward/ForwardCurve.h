#ifndef SJD_OBJ_FUTURESCURVE
#define SJD_OBJ_FUTURESCURVE

#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFwdVol.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticSpotVol.h>
#include <sjd/rates/Forward/Deterministic/ForwardCurveCMADeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveCMAStochasticFwdVol.h>
#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministicFX.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFX.h>
#include <sjd/sjdObjects/RateSource/Forward/ForwardRateSource.h>
#include <sjd/sjdObjects/RateSource/Volatility/VolatilityRateSource.h>
#include <sjd/sjdObjects/RateSource/Discount/DiscountRateSource.h>

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

    /*=====================================================================================
    ForwardCurveDeterministic 

    =======================================================================================*/
    class ForwardCurveDeterministic : public ForwardRateSourceDeterministic
    {
    public:
        ForwardCurveDeterministic(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                           Date anchorDate, vector<Date> observationDates, 
                           vector<double> forwards, 
                           sjd::ArrayInterpolatorType type,
                           bool allowExtrapolation,
                           bool permanent);
              
    protected:
        OH_OBJ_CTOR(ForwardCurveDeterministic, ForwardRateSourceDeterministic);
    };

    /*======================================================================================
    ForwardCurveStochasticFwdVol 

    =======================================================================================*/
    class ForwardCurveStochasticFwdVol : public ForwardRateSourceStochastic
    {
    public:
        ForwardCurveStochasticFwdVol(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                           Date anchorDate, vector<Date> observationDates, 
                           vector<double> forwards, 
                           sjd::ArrayInterpolatorType type,
                           bool allowExtrapolation,
                           boost::shared_ptr<VolatilityRateSource> vrs,
                           bool permanent);
              
    protected:
        OH_OBJ_CTOR(ForwardCurveStochasticFwdVol, ForwardRateSourceStochastic);
    };


    /*======================================================================================
    ForwardCurveStochasticSpotVol 

    =======================================================================================*/
    class ForwardCurveStochasticSpotVol : public ForwardRateSourceStochastic
    {
    public:
        ForwardCurveStochasticSpotVol(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                                      Date anchorDate, vector<Date> observationDates, 
                                      vector<double> forwards, 
                                      sjd::ArrayInterpolatorType type,
                                      bool allowExtrapolation,
                                      boost::shared_ptr<VolatilityRateSource> vrs,
                                      bool permanent);
              
    protected:
        OH_OBJ_CTOR(ForwardCurveStochasticSpotVol, ForwardRateSourceStochastic);
    };

    /*======================================================================================
    ForwardCurveCMADeterministic 

    =======================================================================================*/
    class ForwardCurveCMADeterministic : public ForwardRateSourceDeterministic
    {
    public:
        ForwardCurveCMADeterministic(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                                       Date anchorDate, 
                                       vector<Date> observationDates, 
                                       vector<double> forwards, 
                                       sjd::ArrayInterpolatorType type,
                                       bool allowExtrapolation,
                                       bool permanent);
              
    protected:
        OH_OBJ_CTOR(ForwardCurveCMADeterministic, ForwardRateSourceDeterministic);
    };

    /*======================================================================================
    ForwardCurveCMAStochastic 

    =======================================================================================*/
    class ForwardCurveCMAStochastic : public ForwardRateSourceStochastic
    {
    public:
        ForwardCurveCMAStochastic(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                                  Date anchorDate, 
                                  vector<Date> observationDates, 
                                  vector<double> forwards, 
                                  sjd::ArrayInterpolatorType type,
                                  bool allowExtrapolation,
                                  boost::shared_ptr<VolatilityRateSource> vrs,
                                  bool permanent);              

    protected:
        OH_OBJ_CTOR(ForwardCurveCMAStochastic, ForwardRateSourceStochastic);
    };



    /*=====================================================================================
    ForwardCurveDeterministicFX

    =======================================================================================*/
    class ForwardCurveDeterministicFX : public ForwardRateSourceDeterministic
    {
    public:
        ForwardCurveDeterministicFX(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                                    Date anchorDate, 
                                    double spot,
                                    Date spotDate,
                                    boost::shared_ptr<DiscountRateSource> domesticDRS,
                                    boost::shared_ptr<DiscountRateSource> foreignDRS,
                                    bool permanent);
              
    protected:
        OH_OBJ_CTOR(ForwardCurveDeterministicFX, ForwardRateSourceDeterministic);
    };

    /*======================================================================================
    ForwardCurveStochasticFX 

    =======================================================================================*/
    class ForwardCurveStochasticFX : public ForwardRateSourceStochastic
    {
    public:
        ForwardCurveStochasticFX(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                                    Date anchorDate, 
                                    double spot,
                                    Date spotDate,
                                    boost::shared_ptr<DiscountRateSource> domesticDRS,
                                    boost::shared_ptr<DiscountRateSource> foreignDRS,
                                    boost::shared_ptr<VolatilityRateSource> vrs,
                                    bool permanent);
              
    protected:
        OH_OBJ_CTOR(ForwardCurveStochasticFX, ForwardRateSourceStochastic);
    };

}

#endif
