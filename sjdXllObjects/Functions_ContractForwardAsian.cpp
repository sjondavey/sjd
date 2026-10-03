
#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <sjd/contracts/AsianForwardContract.h>
#include <Tools/vo_GenericUnimplemented.h>
#include <sjd/sjdObjects/Contracts/Contracts.h>

#include <sjd/sjdObjects/RateSource/Discount/DiscountRateSource.h>
#include <sjd/sjdObjects/RateSource/Forward/ForwardRateSource.h>
#include <sjd/sjdObjects/RateSource/Volatility/VolatilityRateSource.h>

#include <ql/time/calendars/all.hpp>
#include <sjd/sjdXllObjects/ObjectCommon.h>


DLLEXPORT char *createContractForwardAsian(char *objectID,
                                            OPER *startOfAveraging,
                                            OPER *endOfAveraging,
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
            (new ObjectHandler::FunctionCall("createContractForwardAsian"));

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
            ObjectHandler::ConvertOper(*settlement), "SettlementDate");

        if (SellementDateLib < EndDateLib)
      {
         ObjectHandler::stringToChar("Settlement < end of averaging!", ret);
         return ret;
      }

        // Strip the Excel cell update counter suffix from Object IDs
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, *permanent));
        
        Calendar calendar = UnitedKingdom();
        boost::shared_ptr<ObjectHandler::Object> object(
            new sjdObjects::AsianForwardContract(valueObject,
                                          calendar,
                                          StartDateLib,
                                          EndDateLib,
                                          SellementDateLib, 
                                          strike, 
                                          volume,
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


