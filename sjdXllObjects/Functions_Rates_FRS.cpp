#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveCMAStochasticFwdVol.h>
#include <sjd/sjdObjects/RateSource/Forward/vo_FuturesCurve.h>
#include <Tools/vo_GenericUnimplemented.h>
#include <sjd/sjdObjects/RateSource/Forward/ForwardCurve.h>

#include <sjd/sjdXllObjects/ObjectCommon.h>

/*======================================================================================
Any object which is derived from sjdObjects::ForwardRateSource can be queried
for its (futures / forward) value using this method
=======================================================================================*/
DLLEXPORT OPER *getForward(char *ObjectId, OPER *XValues, OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getForward"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        // convert input datatypes to QuantLib datatypes
        std::vector<QuantLib::Date> XValuesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*XValues, "XValues");
        if (XValuesLib.size() == 0)
        {
            ObjectHandler::scalarToOper("No input date(s)", returnValue);        
            return &returnValue;
        }

        // convert input datatypes to Object references
        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::ForwardRateSource)
    
        if (!ObjectIdLibObjPtr->isInRange(XValuesLib.back()))
        {
            ObjectHandler::scalarToOper("Final Date not in interpolation range", returnValue);        
            return &returnValue;
        }

        if (!ObjectIdLibObjPtr->isInRange(XValuesLib.front())) 
        {
            ObjectHandler::scalarToOper("Initial Date not in interpolation range", returnValue);        
            return &returnValue;
        }

        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessages(), returnValue);        
        }
        else 
        {
            ObjectHandler::vectorToOper(ObjectIdLibObjPtr->getForward(XValuesLib), returnValue);        
        }
        return &returnValue;
    } 
   SJD_XLL_CATCH_OPER()
}

/*======================================================================================
Create an instance of the object sjdObjects::ForwardCurveStochasticFwdVol and return 
its ID
=======================================================================================*/
DLLEXPORT char *createForwardCurve(char *objectID,
                                   OPER *anchorDate,
                                   OPER *dates,
                                   OPER *futures,
                                   OPER *interpolation,
                                   bool *cma,
                                   bool *stochastic,
                                   char *vrsID,
                                   bool *useSpotVol,
                                   bool *permanent) 
{

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createForwardCurve"));

        QuantLib::Date AnchorDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*anchorDate), "AnchorDate");

        std::vector<QuantLib::Date> CurveDatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "CurveDates");

        std::vector<double> FuturesCpp =
            ObjectHandler::operToVector<double>(*futures, "Futures");

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
        // Strip the Excel cell update counter suffix from Object IDs        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);
        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        std::string returnValue;
        if (!(*stochastic))
        {
            sjdObjects::ForwardRateSourceDeterministic *sfc;                
            if (!(*cma))
            {
                sfc = new sjdObjects::ForwardCurveDeterministic(valueObject, 
                                                                AnchorDateLib,
                                                                CurveDatesLib, 
                                                                FuturesCpp, 
                                                                sjdType, 
                                                                false, 
                                                                *permanent);
            }
            else 
            {
                sfc = new sjdObjects::ForwardCurveCMADeterministic(valueObject,
                                                                   AnchorDateLib,
                                                                   CurveDatesLib,
                                                                   FuturesCpp,
                                                                   sjdType,
                                                                   false,
                                                                   *permanent);
            }
            boost::shared_ptr<ObjectHandler::Object> object(sfc);
            if (!sfc->isOK()) 
            {
                returnValue = sfc->getErrorMessages();
            }
            else 
            {
                returnValue =
                    ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
            }
        }
        else
        {
            sjdObjects::ForwardRateSourceStochastic *sfc;    
              OH_GET_OBJECT(ObjectIdLibObjPtr, vrsID, sjdObjects::VolatilityRateSource)    
            if (!ObjectIdLibObjPtr->isOK())
            {
                ObjectHandler::stringToChar(ObjectIdLibObjPtr->getErrorMessages(), ret);
                return ret;
            }

            if (*cma)
            {
                if (!(*useSpotVol))
                {
                    sfc = new sjdObjects::ForwardCurveCMAStochastic(valueObject,
                                                                   AnchorDateLib,
                                                                   CurveDatesLib,
                                                                   FuturesCpp,
                                                                   sjdType,
                                                                   false,
                                                                   ObjectIdLibObjPtr,
                                                                   *permanent);
                }
                else 
                {
                    ObjectHandler::stringToChar(returnValue, "Not jet implemenented");
                    return ret;
                }
            }
            else 
            {
                if (!(*useSpotVol))
                {
                    sfc = new sjdObjects::ForwardCurveStochasticFwdVol(valueObject,
                                                                       AnchorDateLib,
                                                                       CurveDatesLib,
                                                                       FuturesCpp,
                                                                       sjdType,
                                                                       false,
                                                                       ObjectIdLibObjPtr,
                                                                       *permanent);
                }
                else 
                {
                    sfc = new sjdObjects::ForwardCurveStochasticSpotVol(valueObject,
                                                                        AnchorDateLib,
                                                                        CurveDatesLib,
                                                                        FuturesCpp,
                                                                        sjdType,
                                                                        false,
                                                                        ObjectIdLibObjPtr,
                                                                        *permanent);
                }
            }
            boost::shared_ptr<ObjectHandler::Object> object(sfc);
            if (!sfc->isOK()) 
            {
                returnValue = sfc->getErrorMessages();
            }
            else 
            {
                returnValue =
                    ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
            }
        }
        // Convert and return the return value
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
   SJD_XLL_CATCH_STRING()
}

DLLEXPORT char *createForwardCurveFX(char *objectID,
                                     OPER *anchorDate,
                                     double spot,
                                     OPER *spotDate,
                                     char *domesticDRSID,
                                     char *foreignDRSID,
                                     bool *stochastic,
                                     char *vrsID,
                                     bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createForwardCurveFX"));

        QuantLib::Date AnchorDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*anchorDate), "AnchorDate");

        QuantLib::Date SpotDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*anchorDate), "SpotDate");

        if (SpotDateLib < AnchorDateLib)
        {
            ObjectHandler::stringToChar("Spot Date cannot preceed Anchor Date", ret);
            return ret;
        }
         
        OH_GET_OBJECT(DomesticDRSIdLibObjPtr, domesticDRSID, sjdObjects::DiscountRateSource)    
        OH_GET_OBJECT(ForeignDRSIdLibObjPtr, foreignDRSID, sjdObjects::DiscountRateSource)    

        // Strip the Excel cell update counter suffix from Object IDs        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);
        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        std::string returnValue;
        if (!(*stochastic))
        {
            sjdObjects::ForwardRateSourceDeterministic *sfc;                
            sfc = new sjdObjects::ForwardCurveDeterministicFX(valueObject, 
                                                              AnchorDateLib,
                                                              spot, 
                                                              SpotDateLib, 
                                                              DomesticDRSIdLibObjPtr, 
                                                              ForeignDRSIdLibObjPtr, 
                                                              *permanent);
            boost::shared_ptr<ObjectHandler::Object> object(sfc);
            if (!sfc->isOK()) 
            {
                returnValue = sfc->getErrorMessages();
            }
            else 
            {
                returnValue =
                    ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
            }
        }
        else
        {
            sjdObjects::ForwardRateSourceStochastic *sfc;    
              OH_GET_OBJECT(VolIdLibObjPtr, vrsID, sjdObjects::VolatilityRateSource)    
            sfc = new sjdObjects::ForwardCurveStochasticFX(valueObject,
                                                           AnchorDateLib,
                                                           spot, 
                                                           SpotDateLib, 
                                                           DomesticDRSIdLibObjPtr, 
                                                           ForeignDRSIdLibObjPtr, 
                                                           VolIdLibObjPtr,
                                                           *permanent);
            boost::shared_ptr<ObjectHandler::Object> object(sfc);
            if (!sfc->isOK()) 
            {
                returnValue = sfc->getErrorMessages();
            }
            else 
            {
                returnValue =
                    ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
            }
        }
        // Convert and return the return value
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
   SJD_XLL_CATCH_STRING()
}




/*======================================================================================
Create an instance of the object sjdObjects::ForwardRateSource using a base curve and 
an spread
=======================================================================================*/
DLLEXPORT char *createFRSByBumping(char *outputObjectID,
                                   char *inputObjectID,
                                   double spread,
                                   bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createFRSByBumping"));

        OH_GET_OBJECT(ObjectIdLibObjPtr, inputObjectID, sjdObjects::ForwardRateSourceDeterministic)    
        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::stringToChar(ObjectIdLibObjPtr->getErrorMessages(), ret);
            return ret;
        }
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(outputObjectID);
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        boost::shared_ptr<sjdObjects::ForwardRateSource> fc = 
                                ObjectIdLibObjPtr->parallelBump(valueObject,
                                                                spread,
                                                                *permanent);
        boost::shared_ptr<ObjectHandler::Object> object(fc);

        std::string returnValue;
        boost::shared_ptr<sjd::ForwardRateSource> sjdFrs;
        fc->getLibraryObject(sjdFrs);
        if (!sjdFrs->isOK()) 
        {
            returnValue = sjdFrs->getErrorMessagesAsString();
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


DLLEXPORT char *rollFRS(char *OutputDRSId, OPER *toDate, char *InputDRSID, bool *permanent) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("rollFRS"));

        QuantLib::Date toDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*toDate), "ToDate");

        OH_GET_OBJECT(ObjectIdLibObjPtr, InputDRSID, sjdObjects::ForwardRateSourceDeterministic)    
        if (!ObjectIdLibObjPtr->isInRange(toDateLib))
        {
            ObjectHandler::stringToChar("ToDate is not in FRS range", ret);        
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


        boost::shared_ptr<sjdObjects::ForwardRateSource> frs = 
            ObjectIdLibObjPtr->rollForward(valueObject, toDateLib, *permanent);
        boost::shared_ptr<ObjectHandler::Object> object(frs);
        boost::shared_ptr<sjd::ForwardRateSource> sjdFrs;
        frs->getLibraryObject(sjdFrs);
        std::string returnValue;
        if (!sjdFrs->isOK()) 
        {
            returnValue = sjdFrs->getErrorMessagesAsString();
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


DLLEXPORT OPER *rollFRSStochastic(char *OutputDRSId, char *InputDRSID, OPER *toDates,  OPER *rvs, bool *permanent) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("rollFRSStochastic"));

        std::vector<QuantLib::Date> datesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*toDates, "Dates");
        if (datesLib.size() == 0)
        {
            ObjectHandler::scalarToOper("No input date(s)", returnValue);        
            return &returnValue;
        }
        std::set<QuantLib::Date> datesSet = std::set<QuantLib::Date>(datesLib.begin(), datesLib.end());
        datesLib = std::vector<QuantLib::Date>(datesSet.begin(), datesSet.end());

        std::vector<double> randomNumbers =
            ObjectHandler::operToVector<double>(*rvs, "Random Numbers");
        if (randomNumbers.size() != datesLib.size())
        {
            ObjectHandler::scalarToOper("Sorted dates without duplicates not the same size as the random number input", returnValue);        
            return &returnValue;
        }

        OH_GET_OBJECT(ObjectIdLibObjPtr, InputDRSID, sjdObjects::ForwardRateSourceStochastic)    
        if (!ObjectIdLibObjPtr->isInRange(datesLib.back()))
        {
            ObjectHandler::scalarToOper("Dates not FRS range", returnValue);        
            return &returnValue;
        }
        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessages(), returnValue);        
            return &returnValue;
        }

        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(OutputDRSId);
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));


        vector<boost::shared_ptr<sjdObjects::ForwardRateSource> > frss = 
            ObjectIdLibObjPtr->rollForward(valueObject, datesSet, randomNumbers, *permanent);
        vector<string> objectNames = vector<string>(frss.size());

        for (size_t i = 0; i < frss.size(); ++i)
        {
            stringstream objDescription;
            objDescription << ObjectIdStrip << "_" << (QuantLib::io::iso_date(datesLib[i]));

            boost::shared_ptr<ObjectHandler::Object> object(frss[i]);
            boost::shared_ptr<sjd::ForwardRateSource> sjdFrs;
            frss[i]->getLibraryObject(sjdFrs);
            std::string returnValue;
            if (!sjdFrs->isOK()) 
            {
                objectNames[i] = sjdFrs->getErrorMessagesAsString();
            }
            else 
            {
                objectNames[i] =
                    ObjectHandler::RepositoryXL::instance().storeObject(objDescription.str(), object, true);
            }
        }
        ObjectHandler::vectorToOper(objectNames, returnValue);
        return &returnValue;
    }
   SJD_XLL_CATCH_OPER()
}

