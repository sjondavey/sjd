
#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <sjd/sjdObjects/Contracts/Portfolio/Insurance/PortfolioInsurance.h>
#include <Tools/vo_GenericUnimplemented.h>
#include <sjd/sjdObjects/RateSource/Discount/DiscountRateSource.h>

#include <boost/algorithm/string.hpp>
#include <sjd/sjdXllObjects/ObjectCommon.h>


/*======================================================================================
Create an instance of the object sjdObjects::SimpleInsuranceWithTermStructure and return 
its ID
=======================================================================================*
DLLEXPORT char *createPortfolioInsuranceSimple(char *objectID,
                              OPER *policyStartDate, 
                              OPER *policyMaturityDate, 
                                        OPER *insuredDates, 
                              OPER *insuredAmounts,
                              char *standbyDRSId,
                              char *utilisationDRSId,
                              bool *permanent) 
{

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createPortfolioInsuranceSimple"));

        OH_GET_OBJECT(standbyPtr, standbyDRSId, sjdObjects::DiscountRateSource)
        OH_GET_OBJECT(utilPtr, utilisationDRSId, sjdObjects::DiscountRateSource)

        QuantLib::Date PolicyStartDate = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*policyStartDate), "PolicyStartDate");
        QuantLib::Date PolicyMaturityDate = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*policyMaturityDate), "PolicyMaturityDate");

        std::vector<double> InsuredAmountsCpp =
            ObjectHandler::operToVector<double>(*insuredAmounts, "InsuredAmounts");
        std::vector<Date> InsuredDatesCpp =
            ObjectHandler::operToVector<Date>(*insuredDates, "InsuredDates");

        // Strip the Excel cell update counter suffix from Object IDs        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, *permanent));

        sjdObjects::SimpleInsuranceWithTermStructure *insurance = new sjdObjects::SimpleInsuranceWithTermStructure(
                valueObject,
                PolicyStartDate, 
                PolicyMaturityDate,
                InsuredDatesCpp, 
                InsuredAmountsCpp,
                standbyPtr, 
                utilPtr,
                *permanent);

        boost::shared_ptr<ObjectHandler::Object> object(insurance);
        std::string returnValue;
        boost::shared_ptr<sjd::SimpleInsuranceWithTermStructure> sjdInsurance;
        insurance->getLibraryObject(sjdInsurance);
        if (!sjdInsurance->isOK()) 
      {
            returnValue = sjdInsurance->errorTracking->getErrorMessagesAsString();
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
Any object which is derived from sjdObjects::PortfolioInsurance can be queried for the 
maximum sum insured on any date
=======================================================================================*
DLLEXPORT OPER *getInsuredMaximum(char *ObjectId, OPER *date, OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getInsuredMaximum"));

        ObjectHandler::validateRange(Trigger, "Trigger");

        // convert input datatypes to Object references

        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::PortfolioInsurance)
    
        if (!ObjectIdLibObjPtr->isOK())
      {
         ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessage(), returnValue);
         return &returnValue;
      }

        QuantLib::Date ValueDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*date), "Date");
        
        ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getSumInsured(ValueDateLib), returnValue);
        return &returnValue;

    } 
    SJD_XLL_CATCH_OPER()
}


/*======================================================================================
Any object which is derived from sjdObjects::PortfolioInsurance can be queried for a 
the insurance payments for a given set of mark to markets
=======================================================================================*
DLLEXPORT OPER *getInsurancePayments(char *ObjectId, OPER *dates, OPER *mtms, OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getInsurancePayments"));

        ObjectHandler::validateRange(Trigger, "Trigger");


        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::PortfolioInsurance)
    
        if (!ObjectIdLibObjPtr->isOK())
      {
         ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessage(), returnValue);
         return &returnValue;
      }

        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "Dates");
        std::vector<double> MtMs =
            ObjectHandler::operToVector<double>(*mtms, "MtM");

        if (DatesLib.size() == 0 || (DatesLib.size() != MtMs.size()))
        {
         ObjectHandler::scalarToOper("Input dates and mtm empty or not of the same size", returnValue);
         return &returnValue;
        }
        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(DatesLib)) 
        {
         ObjectHandler::scalarToOper("Input dates vector not ordered", returnValue);
         return &returnValue;
        }

        ObjectHandler::vectorToOper(ObjectIdLibObjPtr->getInsurancePayments(DatesLib, MtMs), returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}
*/

