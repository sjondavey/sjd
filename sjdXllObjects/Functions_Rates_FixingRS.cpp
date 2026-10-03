#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <sjd/sjdObjects/RateSource/Fixing/FixingCurves.h>
#include <Tools/vo_GenericUnimplemented.h>
//#include <sjd/sjdObjects/RateSource/Fixing/FixingRateSource.h>

#include <sjd/sjdXllObjects/ObjectCommon.h>

/*======================================================================================

=======================================================================================*/
DLLEXPORT char *createDailyFixingCurve(char *objectID, OPER *dates, OPER *futures, bool *permanent) 
{

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(new 
                                ObjectHandler::FunctionCall("createDailyFixingCurve"));

        std::vector<double> FuturesCpp =
            ObjectHandler::operToVector<double>(*futures, "Futures");
        std::vector<QuantLib::Date> CurveDatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "CurveDates");
        // Strip the Excel cell update counter suffix from Object IDs        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);
        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, *permanent));
        // Construct the Object
        sjdObjects::DailyFixingCurve *dfc = new sjdObjects::DailyFixingCurve(valueObject, CurveDatesLib, FuturesCpp, *permanent);
        boost::shared_ptr<ObjectHandler::Object> object(dfc);

        if (!dfc->isOK()) 
        {
            ObjectHandler::stringToChar(dfc->getErrorMessages(), ret);
        }
        else 
        {
            std::string returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
            ObjectHandler::stringToChar(returnValue, ret);
        }
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}

/*======================================================================================

=======================================================================================*/
DLLEXPORT char *createMonthlyFixingCurve(char *objectID,
                                          OPER *dates,
                                          OPER *futures,
                                          bool *permanent) {

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try {

        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(new 
            ObjectHandler::FunctionCall("createMonthlyFixingCurve"));

        std::vector<double> FuturesCpp =
            ObjectHandler::operToVector<double>(*futures, "Futures");
        std::vector<QuantLib::Date> CurveDatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "CurveDates");
        // Strip the Excel cell update counter suffix from Object IDs        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);
        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, *permanent));
        // Construct the Object
        sjdObjects::MonthlyFixingCurve *mfc = new sjdObjects::MonthlyFixingCurve(valueObject, 
                                                                                 CurveDatesLib, 
                                                                                 FuturesCpp, 
                                                                                 *permanent);
        boost::shared_ptr<ObjectHandler::Object> object(mfc);
        if (!mfc->isOK()) 
        {
            ObjectHandler::stringToChar(mfc->getErrorMessages(), ret);
        }
        else 
        {
            std::string returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
            ObjectHandler::stringToChar(returnValue, ret);
        }        
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}

DLLEXPORT OPER *getFix(char *ObjectId, OPER *XValues, OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;

    try {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getFix"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        // convert input datatypes to QuantLib datatypes
        std::vector<QuantLib::Date> XValuesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*XValues, "XValues");

        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(XValuesLib))
        {
            ObjectHandler::scalarToOper("Input dates not strictly increasing", returnValue);        
            return &returnValue;
        }


        // convert input datatypes to Object references

      OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::FixingRateSource)
    
        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessages(), returnValue);        
            return &returnValue;
        }

        vector<double> retrunValues;
        for (size_t i = 0; i < XValuesLib.size(); ++i) 
        {
            if (!ObjectIdLibObjPtr->containsARateSetOn(XValuesLib[i]))
            {
                ObjectHandler::scalarToOper("Not all the input dates have associated rates", returnValue);        
                return &returnValue;        
            }
            else 
            {
                ObjectHandler::vectorToOper(ObjectIdLibObjPtr->getRateSetOn(XValuesLib), returnValue);        
            }
        }
        return &returnValue;

    } 
    SJD_XLL_CATCH_OPER()
}

