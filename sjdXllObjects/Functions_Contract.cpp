
#include <qlo/qladdindefines.hpp>
#include <sjd/contracts/Contracts.h>
#include <sjd/sjdObjects/Contracts/Contracts.h>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <Tools/vo_GenericUnimplemented.h>
#include <sjd/maths/maths.h>

#include <sjd/sjdXllObjects/ObjectCommon.h>

#define XLL_DEC DLLEXPORT


XLL_DEC OPER *getContractValue(char *ObjectId, char *ratesEnvironment, OPER *Trigger)
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
   static XLOPER returnValue;
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
               new ObjectHandler::FunctionCall("getContractValue"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        SET_SESSION_ID

      OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::Contract)
      OH_GET_OBJECT(RatesIdLibObjPtr, ratesEnvironment, sjdObjects::RatesEnvironment)

      if (!ObjectIdLibObjPtr->isOK(RatesIdLibObjPtr)) 
       {
            string msg = ObjectIdLibObjPtr->getErrorMessages();
            unsigned maxStringSizeInExcel = 254;
            if (msg.size() > maxStringSizeInExcel) 
            {
                msg = msg.substr(0,maxStringSizeInExcel-1);
            }
         ObjectHandler::scalarToOper(msg, returnValue);
      }
      else 
      {
           ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getValue(RatesIdLibObjPtr), returnValue);
      }
      return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}


XLL_DEC OPER *describeContract(char *ObjectId, char *ratesEnvironment, OPER *Trigger) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
   static XLOPER returnValue;
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("describeContract"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        SET_SESSION_ID

        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::Contract)
        OH_GET_OBJECT(RatesIdLibObjPtr, ratesEnvironment, sjdObjects::RatesEnvironment)

        string description = (ObjectIdLibObjPtr->toString(RatesIdLibObjPtr).c_str());
        string delimiter = "\n";
        size_t pos = 0;
        vector<string> formattedDescription;
        while ((pos = description.find(delimiter)) != std::string::npos) 
        {
            formattedDescription.push_back(description.substr(0, pos));
            description.erase(0, pos + delimiter.length());
        }
        formattedDescription.push_back(description.substr(0, pos));

        static XLOPER returnValue;
        ObjectHandler::vectorToOper(formattedDescription, returnValue);
        return &returnValue;

    } 
    SJD_XLL_CATCH_OPER()
}


DLLEXPORT char *createMultiContract(char *objectID, OPER *inputContracts, OPER *Trigger, bool *permanent) 
{

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createMultiContract"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        // convert input datatypes to QuantLib datatypes

        std::vector<boost::shared_ptr<sjdObjects::Contract>> contracts;

        std::vector<string> contractIDLib = ObjectHandler::operToVector<string>(*inputContracts, "ContractIDs");
        for (size_t i = 0; i < contractIDLib.size(); ++i) 
      {
            OH_GET_OBJECT(contractIdObjPtr, contractIDLib[i], sjdObjects::Contract)
            contracts.push_back(contractIdObjPtr);
        }

        // Strip the Excel cell update counter suffix from Object IDs
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,*permanent));
        
        boost::shared_ptr<ObjectHandler::Object> object(
            new sjdObjects::MultipleInstrumentContract(valueObject, contracts, *permanent));

        // Store the Object in the Repository

        std::string returnValue =
            ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);

        // Convert and return the return value
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}


DLLEXPORT OPER *createMultiContracts(OPER *inputContracts, OPER *key, OPER *Trigger, bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createMultiContracts"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        // convert input datatypes to QuantLib datatypes
        std::vector<string> contractIDLib = ObjectHandler::operToVector<string>(*inputContracts, "ContractIDs");
        std::vector<string> keyString = ObjectHandler::operToVector<string>(*key, "Key");

        if (contractIDLib.size() != keyString.size()) 
        {
            ObjectHandler::scalarToOper("Contracts and Key do not have the same length", returnValue);
            return &returnValue;
        }
        for (size_t i = 0; i < keyString.size(); ++i) 
        {
            boost::to_lower(keyString[i]);
            boost::trim(keyString[i]);
        }

        std::vector<string> uniqueKeyString = keyString;
        std::sort(uniqueKeyString.begin(), uniqueKeyString.end());
        uniqueKeyString.erase(std::unique(uniqueKeyString.begin(), uniqueKeyString.end()), uniqueKeyString.end());

        std::multimap<std::string, boost::shared_ptr<sjdObjects::Contract>> mmap;
        for (size_t i = 0; i < contractIDLib.size(); ++i) 
        {
            OH_GET_OBJECT(contractIdObjPtr, contractIDLib[i], sjdObjects::Contract)
            mmap.insert(std::make_pair(keyString[i],contractIdObjPtr));
        }

        std::vector<std::string> returnValues;

        for (size_t i = 0; i < uniqueKeyString.size(); ++i) 
        {
            std::pair<std::multimap<std::string, boost::shared_ptr<sjdObjects::Contract>>::iterator, 
                std::multimap<std::string, boost::shared_ptr<sjdObjects::Contract>>::iterator> range;
            range = mmap.equal_range(uniqueKeyString[i]); //equal_range returns pair of iterators

            std::vector<boost::shared_ptr<sjdObjects::Contract>> singlePortfolioContracts;
            singlePortfolioContracts.clear();
            //loop through the iterator pair like regular iterators
            for(std::multimap<std::string, boost::shared_ptr<sjdObjects::Contract>>::iterator it = range.first; it != range.second; ++it) 
            {
                singlePortfolioContracts.push_back(it->second);
            }

            // Construct the Value Object
            boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
                new sjdObjects::ValueObjects::GenericUnimplementedValueObject(uniqueKeyString[i],*permanent));
        
            boost::shared_ptr<ObjectHandler::Object> object(
                new sjdObjects::MultipleInstrumentContract(valueObject, singlePortfolioContracts, *permanent));

            // Store the Object in the Repository
            returnValues.push_back(ObjectHandler::RepositoryXL::instance().storeObject(uniqueKeyString[i], object, true));
        }

        ObjectHandler::vectorToOper(returnValues, returnValue);
        return &returnValue;

    } 
    SJD_XLL_CATCH_OPER()
}


