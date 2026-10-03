
#include <qlo/qladdindefines.hpp>
#include <sjd/contracts/Contracts.h>
#include <sjd/sjdObjects/Contracts/Contracts.h>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <Tools/vo_GenericUnimplemented.h>
#include <sjd/sjdXllObjects/ObjectCommon.h>


#define XLL_DEC DLLEXPORT

DLLEXPORT char *createContractForward(char *objectID,
                                        OPER *rateSetDate,
                                        OPER *settlement,
                                        double strike,
                                        double volume,
                                        char* buySell,
                                        bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
   {

        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createContractForward"));

        sjd::BuySell bs;
      if (!sjdObjects::convertBuySellInput(buySell, bs))
      {
         ObjectHandler::stringToChar("(B)uy or (S)ell not set correctly", ret);
         return ret;
      }
      
        QuantLib::Date RateSetDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*rateSetDate), "RateSetDate");

        QuantLib::Date SellementDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*settlement), "SettlementDate");


        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, *permanent));
        
        boost::shared_ptr<ObjectHandler::Object> object(
            new sjdObjects::ForwardContract(valueObject, RateSetDateLib, SellementDateLib, strike, volume, bs, *permanent));

        std::string returnValue =
            ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);


        ObjectHandler::stringToChar(returnValue, ret);
        return ret;

    } 
   SJD_XLL_CATCH_STRING()
}
