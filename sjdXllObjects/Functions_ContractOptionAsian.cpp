
#include <qlo/qladdindefines.hpp>
#include <sjd/contracts/Contracts.h>
#include <sjd/sjdObjects/Contracts/Contracts.h>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <Tools/vo_GenericUnimplemented.h>

#include <ql/time/calendars/all.hpp>
#include <sjd/sjdXllObjects/ObjectCommon.h>


#define XLL_DEC DLLEXPORT


DLLEXPORT char *createContractOptionAsian(char *objectID,
                                           OPER *startOfAveraging,
                                          OPER *endOfAveraging,
                                          OPER *optionSettlement,
                                          double strike,
                                          char* putCall,
                                          double volume,
                                          OPER *premiumDate,
                                          double premium,
                                          char* buySell,
                                          bool *permanent) 
{

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];

    try 
   {

        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createContractOptionAsian"));

        sjd::PutCall pc;
      if (!sjdObjects::convertPutCallInput(putCall, pc))
      {
         ObjectHandler::stringToChar("(P)ut or (C)all not set correctly", ret);
         return ret;
      }

        sjd::BuySell bs;
      if (!sjdObjects::convertBuySellInput(buySell, bs))
      {
         ObjectHandler::stringToChar("(B)uy or (S)ell not set correctly", ret);
         return ret;
      }

        // convert input datatypes to QuantLib datatypes

        QuantLib::Date StartDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*startOfAveraging), "StartDate");

        QuantLib::Date EndDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*endOfAveraging), "EndDate");

        if (EndDateLib <= StartDateLib)
      {
         ObjectHandler::stringToChar("Start of averaging > end of the averaging!", ret);
         return ret;
      }

        QuantLib::Date SellementDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*optionSettlement), "SettlementDate");

        if (SellementDateLib < EndDateLib)
      {
         ObjectHandler::stringToChar("Settlement < end of averaging!", ret);
         return ret;
      }

        QuantLib::Date PremiumDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*premiumDate), "PremiumPaymentDate");

        // Strip the Excel cell update counter suffix from Object IDs
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, *permanent));
        
        Calendar calendar = UnitedKingdom();
        boost::shared_ptr<ObjectHandler::Object> object(
            new sjdObjects::AsianOptionContract(valueObject,
                                calendar,
                                          StartDateLib,
                                          EndDateLib,
                                          SellementDateLib, 
                                          strike, 
                                          pc,
                                          volume,
                                          PremiumDateLib, 
                                          premium, 
                                          bs,
                                          *permanent));


        // Store the Object in the Repository

        std::string returnValue =
            ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);

        ObjectHandler::stringToChar(returnValue, ret);
        return ret;


    } 
   SJD_XLL_CATCH_STRING()
}

