
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

DLLEXPORT char *createContractOption(char *objectID,
                                OPER *optionMaturity,
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
    try {

        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createContractOption"));

      // convert input datatypes to C++ datatypes

        ObjectHandler::property_t MaturityDateCpp = ObjectHandler::convert2<ObjectHandler::property_t>(
            ObjectHandler::ConvertOper(*optionMaturity));

        ObjectHandler::property_t SettlementDateCpp = ObjectHandler::convert2<ObjectHandler::property_t>(
            ObjectHandler::ConvertOper(*optionSettlement));

        ObjectHandler::property_t PremiumDateCpp = ObjectHandler::convert2<ObjectHandler::property_t>(
            ObjectHandler::ConvertOper(*premiumDate));

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

        QuantLib::Date MaturityDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*optionMaturity), "MaturityDate");

        QuantLib::Date SellementDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*optionSettlement), "SettlementDate");

        QuantLib::Date PremiumDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*premiumDate), "PremiumPaymentDate");


        // Strip the Excel cell update counter suffix from Object IDs
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, *permanent));
        
        // Construct the Object
        //DayCounter dayCounter = Actual365Fixed();


        boost::shared_ptr<ObjectHandler::Object> object(
            new sjdObjects::OptionContract(valueObject,
                             MaturityDateLib, 
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

                               
