
#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <sjd/contracts/Portfolio/CVA/cva.h>
#include <sjd/sjdObjects/Contracts/Portfolio/CVA/CVA.h>

#include <Tools/vo_GenericUnimplemented.h>

#include <boost/algorithm/string.hpp>

#include <sjd/sjdXllObjects/ObjectCommon.h>

/*======================================================================================
Any object which is derived from sjdObjects::CVA be queried for it's LGD
=======================================================================================*/
DLLEXPORT OPER *getCVALGD(char *ObjectId, OPER *date1, OPER *Trigger) 
{    
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getCVALGD"));

        ObjectHandler::validateRange(Trigger, "Trigger");

        QuantLib::Date toDate = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*date1), "ToDate");


        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::CVA)

        if (!ObjectIdLibObjPtr->isOK())
        {
         ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessage(), returnValue);
         return &returnValue;
        }
        ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getLGD(toDate), returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}


/*======================================================================================
Any object which is derived from sjdObjects::CVA can be queried for a Probability of
default between any two dates.
=======================================================================================*/
DLLEXPORT OPER *getCVAPD(char *ObjectId, OPER *date1, OPER *date2, OPER *Trigger) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getCVAPD"));

        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::CVA)
    
        if (!ObjectIdLibObjPtr->isOK())
        {
         ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessage(), returnValue);
         return &returnValue;
        }

        QuantLib::Date startDate = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*date1), "StartDate");
        QuantLib::Date endDate = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*date2), "EndDate");

        if (startDate > endDate)
      {
         ObjectHandler::scalarToOper("Date1 > Date2!", returnValue);
         return &returnValue;
      }

        if (!ObjectIdLibObjPtr->extendsTo(endDate))
      {
         ObjectHandler::scalarToOper("Date2 after end of CVA object", returnValue);
         return &returnValue;
      }
        ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getPD(startDate, endDate), returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}



/*======================================================================================
Create an instance of the object sjdObjects::FlatPDand return its ID
=======================================================================================*/
DLLEXPORT char *createCVAFlatPD(char *objectID, OPER *anchorDate, double pd, double lgd, bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createCVAFlatPD"));

        // Strip the Excel cell update counter suffix from Object IDs        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, *permanent));

        QuantLib::Date ad = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*anchorDate), "AnchorDate");

        sjdObjects::FlatPD *cva = new sjdObjects::FlatPD(valueObject, ad, pd, lgd, *permanent);

        boost::shared_ptr<ObjectHandler::Object> object(cva);
        std::string returnValue;
        if (!cva->isOK()) 
        {
            returnValue = cva->getErrorMessage();
        }
        else 
        {
            // Store the Object in the Repository
            returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
        }
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}

/*======================================================================================
Create an instance of the object sjdObjects::CumulativeDefaultProbability return its ID
=======================================================================================*/
DLLEXPORT char *createCVAFCDF(char *objectID, OPER *anchorDate,  
                              OPER *dates, OPER *cdp, OPER *lgd, bool *permanent) 
{

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createCVAFCDF"));

        // Strip the Excel cell update counter suffix from Object IDs        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, *permanent));

        QuantLib::Date ad = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*anchorDate), "AnchorDate");

        std::vector<Date> CurveDatesCpp =
            ObjectHandler::operToVector<Date>(*dates, "CurveDates");

        std::vector<double> CDPCpp =
            ObjectHandler::operToVector<double>(*cdp, "CDP");

        std::vector<double> LGDCpp =
            ObjectHandler::operToVector<double>(*lgd, "LGD");

        if ((CurveDatesCpp.size() < 1) || (CurveDatesCpp.size() != CDPCpp.size()) || (CurveDatesCpp.size() != LGDCpp.size())) 
      {
         ObjectHandler::stringToChar("Input vectors empty or not the same size", ret);
         return ret;
        }

        sjdObjects::CumulativeDefaultProbability *cva = 
            new sjdObjects::CumulativeDefaultProbability(valueObject, ad, CurveDatesCpp, CDPCpp, LGDCpp, *permanent);

        boost::shared_ptr<ObjectHandler::Object> object(cva);
        std::string returnValue;
        if (!cva->isOK()) 
        {
            returnValue = cva->getErrorMessage();
        }
        else 
        {
            // Store the Object in the Repository
            returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
        }
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}

