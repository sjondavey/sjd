
#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <sjd/rates/Volatility/VolatilityCurve.h>
#include <sjd/sjdObjects/RateSource/Volatility/vo_VolatilityCurve.h>
#include <sjd/sjdObjects/RateSource/Volatility/VolatilityCurve.h>
#include <sjd/sjdObjects/RateSource/Volatility/VolatilitySurfaces.h>

#include <Tools/vo_GenericUnimplemented.h>

#include <sjd/sjdXllObjects/ObjectCommon.h>

/*======================================================================================
Any object which is derived from sjdObjects::VolatilityRateSource can be queried
for its volatility using this method
=======================================================================================*/
DLLEXPORT OPER *getVolatility(char *ObjectId, OPER *date, bool *returnAsStandardDeviation, OPER *Trigger)
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getVolatility"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        // convert input datatypes to QuantLib datatypes
        QuantLib::Date dateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*date), "Date");


        // convert input datatypes to Object references
        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::VolatilityRateSource)

        if (!ObjectIdLibObjPtr->isInRange(dateLib))
        {
            ObjectHandler::scalarToOper("Date not in interpolation range", returnValue);        
            return &returnValue;
        }

        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessages(), returnValue);
        }
        else 
        {
            if (*returnAsStandardDeviation)
            {
                ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getStandardDeviation(dateLib), returnValue);                
            }

            else 
            {
                ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getVolatility(dateLib), returnValue);                
            }
        }
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

/*======================================================================================
Any object which is derived from sjdObjects::VolatilityRateSource can be queried
for its volatility using this method
=======================================================================================*/
DLLEXPORT OPER *getVolatilityForStrike(char *ObjectId,
                                         OPER *date,
                                         double strike,
                                         double forward,
                                         bool *returnAsStandardDeviation,
                                         OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getVolatilityForStrike"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        // convert input datatypes to QuantLib datatypes
        QuantLib::Date dateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*date), "Date");

        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::VolatilityRateSource)

        if (!ObjectIdLibObjPtr->isInRange(dateLib, strike, forward))
        {
            ObjectHandler::scalarToOper("Date, Delta not in interpolation range", returnValue);        
            return &returnValue;
        }

        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessages(), returnValue);
        }
        else 
        {
            if (*returnAsStandardDeviation)
            {
                ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getStandardDeviation(dateLib, strike, forward), returnValue);                
            }

            else 
            {
                ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getVolatility(dateLib, strike, forward), returnValue);                
            }
        }
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}


DLLEXPORT char *createVolCurve(char *objectID,
                               OPER *anchorDate,
                               OPER *dates,
                               OPER *volatility,
                               OPER *dayCounter,
                               OPER *interpolation,
                               bool *cma,
                               bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createVolCurve"));

        // convert input datatypes to C++ datatypes

        ObjectHandler::property_t AnchorDateCpp = ObjectHandler::convert2<ObjectHandler::property_t>(
            ObjectHandler::ConvertOper(*anchorDate));

        std::vector<ObjectHandler::property_t> CurveDatesCpp =
            ObjectHandler::operToVector<ObjectHandler::property_t>(*dates, "CurveDates");

        std::vector<double> VolatilityCpp =
            ObjectHandler::operToVector<double>(*volatility, "Volatilities");

        std::string DayCounterCpp = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*dayCounter), "DayCounter", "Actual/365 (Fixed)");

        std::string InterpolationType = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*interpolation), "Interpolation");
        sjd::ArrayInterpolatorType sjdType; // = sjd::LINEAR;
        boost::to_lower(InterpolationType);
        boost::trim(InterpolationType);
        if (InterpolationType.compare("linear") == 0) 
        {
            sjdType = sjd::LINEAR;
        }
        else if (InterpolationType.compare("cubic") == 0) 
        {
            sjdType = sjd::CUBIC;
        }
        else 
        {
            ObjectHandler::stringToChar("Interpolator type must be either LINEAR or CUBIC", ret);
            return ret;
        }
        // convert input datatypes to QuantLib datatypes

        QuantLib::Date AnchorDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*anchorDate), "AnchorDate");

        std::vector<QuantLib::Date> CurveDatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "CurveDates");

        if ((CurveDatesLib.size() == 0) || (CurveDatesLib.size() == 0))
        {
            ObjectHandler::stringToChar("Either the dates or the vols are empty", ret);        
            return ret;
        }

        if (CurveDatesLib.size() != VolatilityCpp.size())
        {
            ObjectHandler::stringToChar("Dates and volatiltiy inputs to not have the same dimension", ret);        
            return ret;
        }

        // convert input datatypes to QuantLib enumerated datatypes

        QuantLib::DayCounter DayCounterEnum =
            ObjectHandler::Create<QuantLib::DayCounter>()(DayCounterCpp);

        // Strip the Excel cell update counter suffix from Object IDs
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));
        // Construct the Object
        

        sjdObjects::DeterministicVolatilityRateSource *vc;
        
        if (!*cma)
        {
            vc = new sjdObjects::VolatilityCurve(valueObject, 
                                                 AnchorDateLib,
                                                 CurveDatesLib, 
                                                 VolatilityCpp, 
                                                 DayCounterEnum, 
                                                 sjdType, 
                                                 false, 
                                                 *permanent);
        }
        else
        {
            vc = new sjdObjects::VolatilityCurveCMA(valueObject, 
                                                    AnchorDateLib,
                                                    CurveDatesLib, 
                                                    VolatilityCpp, 
                                                    DayCounterEnum, 
                                                    sjdType, 
                                                    false, 
                                                    *permanent);
        }

        boost::shared_ptr<ObjectHandler::Object> object(vc);
        if (!vc->isOK()) 
        {
            ObjectHandler::stringToChar(vc->getErrorMessages(), ret);
        }
        else 
        {
            // Store the Object in the Repository
            std::string returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
            ObjectHandler::stringToChar(returnValue, ret);
        }
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}


DLLEXPORT char *createVolSurface(char *objectID,
                                 OPER *anchorDate,
                                 OPER *dates,
                                 OPER *dimension,
                                 OPER *data,
                                 OPER *interpolation,
                                 OPER *dimensionType,
                                 bool *cma,
                                 bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {

        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createVolSurface"));

        // convert input datatypes to C++ datatypes
        // Not necessary as I've not created a propper value object

        // convert input datatypes to QuantLib datatypes

        QuantLib::Date AnchorDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*anchorDate), "AnchorDate");

        std::vector<QuantLib::Date> datesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "Dates");

        std::vector<double> dimensionLib =
            ObjectHandler::operToVector<double>(*dimension, "Dimension Data");
        std::string dimenssionDescription = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*dimensionType), "Dimension Type");
        boost::to_lower(dimenssionDescription);
        boost::trim(dimenssionDescription);
        bool moneyness = false;
        bool delta = false;
        if (dimenssionDescription.compare("moneyness") == 0) 
        {
            moneyness = true;
        }
        else if (dimenssionDescription.compare("delta") == 0) 
        {
            delta = true;
        }
        else 
        {
            ObjectHandler::stringToChar("Interpolator type must be either MONEYNESS or DELTA", ret);
            return ret;
        }
        if (delta && dimensionLib.back() < 1.0)
        {
            ObjectHandler::stringToChar("Input DELTA must be beteween 0 and 100, not 0 and 1", ret);
            return ret;
        }

        vector<vector<double>> volatilityLib = ObjectHandler::operToMatrix<double>(*data, "Volatility Data");


        std::string InterpolationType = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*interpolation), "Interpolation");
        SurfaceInterpolatorType sjdType; // = sjd::LINEAR;
        boost::to_lower(InterpolationType);
        boost::trim(InterpolationType);
        if (InterpolationType.compare("bilinear") == 0) 
        {
            sjdType = BILINEAR;
        }
        else if (InterpolationType.compare("bicubic") == 0) 
        {
            sjdType = BICUBIC;
        }
        else 
        {
            ObjectHandler::stringToChar("Interpolator type must be either BILINEAR or BICUBIC", ret);
            return ret;
        }

        // Strip the Excel cell update counter suffix from Object IDs
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::CalendarAverageVolatilityMoneynessSurfaceValueObject(
                ObjectIdStrip,
                *permanent));


        sjdObjects::DeterministicVolatilityRateSource *vs;
        if (moneyness)
        {
            if (*cma)
            {
                vs = new sjdObjects::VolatilitySurfaceMoneynessCMA(valueObject, 
                                                                   AnchorDateLib, 
                                                                   datesLib, 
                                                                   dimensionLib, 
                                                                   volatilityLib, 
                                                                   sjdType,
                                                                   *permanent);
            }
            else
            {
                ObjectHandler::stringToChar("European Moneyness Surface not yet implemented", ret);
                return ret;
            }
        }
        else if (delta)
        {
            if (*cma)
            {
                ObjectHandler::stringToChar("Calendar Month Average Delta Surface not yet implemented", ret);
                return ret;
            }
            else 
            {
                vs = new sjdObjects::VolatilitySurfaceDelta(valueObject,
                                                            AnchorDateLib, 
                                                            datesLib, 
                                                            dimensionLib, 
                                                            volatilityLib, 
                                                            sjdType,
                                                            *permanent);
            }
        }
        else
        {
            ObjectHandler::stringToChar("Interpolator type must be either MONEYNESS or DELTA", ret);
            return ret;
        }

        boost::shared_ptr<ObjectHandler::Object> object(vs);
        
        if (!vs->isOK()) 
        {
            ObjectHandler::stringToChar(vs->getErrorMessages(), ret);
        }
        else {
            // Store the Object in the Repository
            std::string returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
            ObjectHandler::stringToChar(returnValue, ret);
        }
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}


/*======================================================================================
Create an instance of the object sjdObjects::VolatilityRateSource using a base curve and 
an spread
=======================================================================================*/
DLLEXPORT char *createVRSByBumping(char *outputObjectID,
                                   char *inputObjectID,
                                   double spread,
                                   bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createVRSByBumping"));

        if ((spread > .1) || (spread < -.1))
        {
            ObjectHandler::stringToChar("Volatility rate spread must be between -10% and 10%", ret);
            return ret;
        }
        OH_GET_OBJECT(ObjectIdLibObjPtr, inputObjectID, sjdObjects::DeterministicVolatilityRateSource)    
        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::stringToChar(ObjectIdLibObjPtr->getErrorMessages(), ret);
            return ret;
        }
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(outputObjectID);
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        boost::shared_ptr<sjdObjects::VolatilityRateSource> vc = 
                ObjectIdLibObjPtr->parallelBump(valueObject,
                                                spread,
                                                *permanent);
        boost::shared_ptr<ObjectHandler::Object> object(vc);

        std::string returnValue;
        boost::shared_ptr<sjd::VolatilityRateSource> sjdVrs;
        vc->getLibraryObject(sjdVrs);
        if (!sjdVrs->isOK()) 
        {
            returnValue = sjdVrs->getErrorMessagesAsString();
        }
        else 
        {
            // Store the Object in the Repository
            returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
        }
        // Convert and return the return value
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
   SJD_XLL_CATCH_STRING()
}


DLLEXPORT char *rollVRS(char *OutputDRSId, OPER *toDate, char *InputDRSID, bool *permanent) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("rollVRS"));

        QuantLib::Date toDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*toDate), "ToDate");

        OH_GET_OBJECT(ObjectIdLibObjPtr, InputDRSID, sjdObjects::DeterministicVolatilityRateSource)    
        if (!ObjectIdLibObjPtr->isInRange(toDateLib))
        {
            ObjectHandler::stringToChar("ToDate is not in VRS range", ret);        
            return ret;
        }
        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::stringToChar(ObjectIdLibObjPtr->getErrorMessages(), ret);        
            return ret;
        }

        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(OutputDRSId);
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));


        boost::shared_ptr<sjdObjects::VolatilityRateSource> vrs = 
            ObjectIdLibObjPtr->rollForward(valueObject, toDateLib, *permanent);
        boost::shared_ptr<ObjectHandler::Object> object(vrs);
        boost::shared_ptr<sjd::VolatilityRateSource> sjdVrs;
        vrs->getLibraryObject(sjdVrs);
        std::string returnValue;
        if (!sjdVrs->isOK()) 
        {
            returnValue = sjdVrs->getErrorMessagesAsString();
        }
        else 
        {
            returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
        }
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    }
   SJD_XLL_CATCH_STRING()
}

