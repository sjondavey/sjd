
#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <sjd/contracts/Portfolio/Margin/Margin.h>
#include <sjd/sjdObjects/Contracts/Portfolio/Margin/Margin.h>
#include <Tools/vo_GenericUnimplemented.h>

#include <boost/algorithm/string.hpp>

#include <sjd/sjdXllObjects/ObjectCommon.h>

/*======================================================================================
Create an instance of the object sjdObjects::SimpleMargin and return its ID
=======================================================================================*/
DLLEXPORT char *createMarginSimple(char *objectID,
                                    OPER *anchorDate,
                                    bool *useMyThreshold, 
                                    double myThreshold,
                                    bool *useCptyThreshold, 
                                    double cptyThreshold,
                                    double minimumTransferAmount, 
                                    double rounding,
                                    bool *permanent) 
{

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createMarginSimple"));

        // Strip the Excel cell update counter suffix from Object IDs        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, *permanent));

        QuantLib::Date AnchorDate = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*anchorDate), "AnchorDate");


        sjdObjects::SimpleMargin *margin = new sjdObjects::SimpleMargin(valueObject,
                                                                        AnchorDate,                                                                        
                                                                        *useMyThreshold, 
                                                                        myThreshold,
                                                                        *useCptyThreshold, 
                                                                        cptyThreshold,
                                                                        minimumTransferAmount, 
                                                                        rounding,
                                                                        *permanent);

        boost::shared_ptr<ObjectHandler::Object> object(margin);
        if (!margin->isOK()) 
        {
            ObjectHandler::stringToChar(margin->getErrorMessage(), ret);
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
Any object which is derived from sjdObjects::Margin can have its current balance set
using this method
=======================================================================================*/
DLLEXPORT OPER *setMarginBalance(char *ObjectId, double balance, OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("setMarginBalance"));

        ObjectHandler::validateRange(Trigger, "Trigger");

        // convert input datatypes to Object references

      OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::Margin)
    
        if (!ObjectIdLibObjPtr->isOK())
      {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessage(), returnValue);        
            return &returnValue;
      }
        ObjectIdLibObjPtr->setMarginBalance(balance);        
        
        ObjectHandler::scalarToOper(true, returnValue);
        return &returnValue;

    } 
    SJD_XLL_CATCH_OPER()
}


/*======================================================================================
Any object which is derived from sjdObjects::Margin can be queried for a margin call
using this method.
=======================================================================================*/
DLLEXPORT OPER *getMarginCall(char *ObjectId, double openingBalance, double mtm, OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getMarginCall"));

        ObjectHandler::validateRange(Trigger, "Trigger");

        // convert input datatypes to Object references

         OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::Margin)

    
        if (!ObjectIdLibObjPtr->isOK())
      {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessage(), returnValue);        
            return &returnValue;
      }

        double balance = ObjectIdLibObjPtr->getMarginBalance();
        ObjectIdLibObjPtr->setMarginBalance(openingBalance);
        double marginCall = ObjectIdLibObjPtr->getMarginCall(mtm);
        ObjectIdLibObjPtr->setMarginBalance(balance); // reset the object's opening balance

        ObjectHandler::scalarToOper(marginCall, returnValue);
        return &returnValue;

    } 
    SJD_XLL_CATCH_OPER()
}
