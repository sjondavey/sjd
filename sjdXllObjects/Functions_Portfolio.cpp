#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <sjd/contracts/Portfolio/Portfolio.h>
#include <Tools/vo_GenericUnimplemented.h>
#include <sjd/sjdObjects/Contracts/Portfolio/Portfolio.h>
#include <sjd/sjdObjects/Contracts/Portfolio/Margin/Margin.h>

#include <sjd/sjdObjects/RateSource/Discount/DiscountRateSource.h>
#include <sjd/sjdObjects/RateSource/Forward/ForwardRateSource.h>
#include <sjd/sjdObjects/RateSource/Volatility/VolatilityRateSource.h>

//#include "vectorOfVectorsToOper.h"
#include <sjd/sjdXllObjects/ObjectCommon.h>


DLLEXPORT OPER *getPortfolioMarginBalance(char *PortfolioObjectId, OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getPortfolioMarginBalance"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(PortfolioObjectIdPtr, PortfolioObjectId, sjdObjects::Portfolio)        
        if (PortfolioObjectIdPtr->isMargined())
        {
            double marginBalance = PortfolioObjectIdPtr->getMarginBalance();
            ObjectHandler::scalarToOper(marginBalance, returnValue);
        }
        else
        {
            ObjectHandler::scalarToOper("Portfolio is not margined", returnValue);
        }
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getPortfolioMarginCall(char *PortfolioObjectId, char *ratesEnvironment, OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getPortfolioMarginCall"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(PortfolioObjectIdPtr, PortfolioObjectId, sjdObjects::Portfolio)        
        if (PortfolioObjectIdPtr->isMargined())
        {
            OH_GET_OBJECT(reObjectIdPtr, ratesEnvironment, sjdObjects::RatesEnvironment)
            double marginCall = PortfolioObjectIdPtr->getMarginCall(reObjectIdPtr);
            ObjectHandler::scalarToOper(marginCall, returnValue);
        }
        else
        {
            ObjectHandler::scalarToOper("Portfolio is not margined", returnValue);
        }
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

DLLEXPORT char *createPortfolio(char *objectID, OPER *inputContracts, char *marginID, OPER *Trigger, bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createPortfolio"));
        ObjectHandler::validateRange(Trigger, "Trigger");

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
        

        boost::shared_ptr<ObjectHandler::Object> object;
        if (strlen(marginID) > 0)
        {
            OH_GET_OBJECT(marginObjectIdLibObjPtr, marginID, sjdObjects::Margin);
            object = boost::shared_ptr<ObjectHandler::Object>(new sjdObjects::Portfolio(valueObject, contracts, marginObjectIdLibObjPtr, *permanent));
        }
        else 
        {
            object = boost::shared_ptr<ObjectHandler::Object>(new sjdObjects::Portfolio(valueObject, contracts, *permanent));
        }

        // Store the Object in the Repository

        std::string returnValue =
            ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}


DLLEXPORT OPER *getPortfolioRegCap(char *ObjectId, char *ratesEnvironment, OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getPortfolioRegCap"));
        ObjectHandler::validateRange(Trigger, "Trigger");
        // initialize the session ID (if enabled)
        SET_SESSION_ID
        // convert object IDs into library objects

        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::PortfolioWithRegulatoryCapital)

        OH_GET_OBJECT(reIdLibObjPtr, ratesEnvironment, sjdObjects::RatesEnvironment)

        if (!ObjectIdLibObjPtr->isOK(reIdLibObjPtr)) 
          {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessages(), returnValue);
            return &returnValue;
        }
        ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getRegulatoryCapital(reIdLibObjPtr), returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

DLLEXPORT char *createPortfolioB2CEM(char *objectID, 
                                     OPER *contractsInputs, 
                                     char *factor, 
                                     double cpw,  
                                     double cc, 
                                     bool *optimise, 
                                     char *margin,
                                     OPER *Trigger, 
                                     bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {

        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createPortfolioB2CEM"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        string factorDescription(factor);
        boost::shared_ptr<sjd::RegCapUnderlyingFactors> regCapFactors = sjd::generateRegCapFactors(factorDescription);
        if (regCapFactors == NULL) 
        {
            ObjectHandler::stringToChar("Factors must be one of IR, FX, Equity, Precious, Commodity", ret);
            return ret;
        }
        if (cpw <= 0)
        {
            ObjectHandler::stringToChar("Counterparty weight must be > 0", ret);
            return ret;
        }
        if (cc <= 0)
        {
            ObjectHandler::stringToChar("Capital charge must be > 0", ret);
            return ret;
        }
        boost::shared_ptr<sjd::RegCapCreditParameters> param = 
            boost::shared_ptr<sjd::RegCapCreditParameters>(new sjd::RegCapCreditParameters(cpw, cc));

        std::vector<boost::shared_ptr<sjdObjects::Contract>> contracts;
        std::vector<string> contractIDLib = ObjectHandler::operToVector<string>(*contractsInputs, "ContractIDs");
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
        
        boost::shared_ptr<ObjectHandler::Object> object;
        if (strlen(margin) > 0)
        {        
            OH_GET_OBJECT(marginObjectIdLibObjPtr, margin, sjdObjects::Margin);
            object = boost::shared_ptr<ObjectHandler::Object>(
                new sjdObjects::PortfolioB2CEM(valueObject, contracts, regCapFactors, param, *optimise, marginObjectIdLibObjPtr, *permanent));
        }
        else
        {
            object = boost::shared_ptr<ObjectHandler::Object>(
                new sjdObjects::PortfolioB2CEM(valueObject, contracts, regCapFactors, param, *optimise, *permanent));
        }

        // Store the Object in the Repository
        std::string returnValue =
            ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);

        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}

/*
DLLEXPORT char *createPortfolioB3CEM(char *objectID, 
                                     OPER *contractsInputs, 
                                     char *factor, 
                                     double cpw,  
                                     double cc, 
                                     char* cr, 
                                     bool *optimise, 
                                     OPER *Trigger, 
                                     bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];

    try {

        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createPortfolioB3CEM"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        string factorDescription(factor);
        boost::shared_ptr<sjd::RegCapUnderlyingFactors> regCapFactors = sjd::generateRegCapFactors(factorDescription);
        if (regCapFactors == NULL) 
        {
            ObjectHandler::stringToChar("Factors must be one of IR, FX, Equity, Precious, Commodity", ret);
            return ret;
        }
        if (cpw <= 0)
        {
            ObjectHandler::stringToChar("Counterparty weight must be > 0", ret);
            return ret;
        }
        if (cc <= 0)
        {
            ObjectHandler::stringToChar("Capital charge must be > 0", ret);
            return ret;
        }
        boost::shared_ptr<sjd::RegCapCreditParameters> param = 
            boost::shared_ptr<sjd::RegCapCreditParameters>(new sjd::RegCapCreditParameters(cpw, cc));

        string creditRatingString(cr);
        boost::shared_ptr<sjd::B3CreditRating> creditRating = 
            boost::shared_ptr<sjd::B3CreditRating>(new sjd::B3CreditRating(creditRatingString));
        
        if (!creditRating->isOK()) 
        {
            ObjectHandler::stringToChar("Credit rating must be either AAA, AA, A, BBB, BB, B or C", ret);
            return ret;
        }

        std::vector<boost::shared_ptr<sjdObjects::Contract>> contracts;
        std::vector<string> contractIDLib = ObjectHandler::operToVector<string>(*contractsInputs, "ContractIDs");
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
            new sjdObjects::PortfolioB3CEM(valueObject, contracts, regCapFactors, param, creditRating, *optimise, *permanent));

        // Store the Object in the Repository
        std::string returnValue =
            ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);

        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}

DLLEXPORT OPER *getSimDatesForDates(char *PortfolioObjectId,                                   
                            OPER *exposureDates,
                                    OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getSimDatesForDates"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*exposureDates, "Dates");

        OH_GET_OBJECT(PortfolioObjectIdPtr, PortfolioObjectId, sjdObjects::Portfolio)

      std::vector<Date> simDates = PortfolioObjectIdPtr->getSimDatesForDates(DatesLib);

        ObjectHandler::vectorToOper(simDates, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}


DLLEXPORT OPER *getPortfolioMarginAlongPath(char *ObjectId,
                                            OPER *dates,
                                            OPER *spotRates,
                                            OPER *Trigger) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getPortfolioMarginAlongPath"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "Dates");
        std::vector<double> rates =
            ObjectHandler::operToVector<double>(*spotRates, "SpotRates");
        std::vector<double> values = std::vector<double>(rates.size(), numeric_limits<double>::quiet_NaN());
        if (DatesLib.size() != rates.size())
        {
         ObjectHandler::scalarToOper("Input dates and rates do not have the same size", returnValue);
         return &returnValue;
        }
        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(DatesLib)) 
        {
         ObjectHandler::scalarToOper("Input dates vector not ordered", returnValue);
         return &returnValue;
        }

        // convert object IDs into library objects
        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::Portfolio)
        values = ObjectIdLibObjPtr->getMarginBalanceAlongPath(DatesLib, rates);
        
        // invoke the member function
        if (!ObjectIdLibObjPtr->areValuationDatesOK()) 
      {
         ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessages(), returnValue);
         return &returnValue;
      }

        ObjectHandler::vectorToOper(values, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()

}

DLLEXPORT OPER *runPortfolioSimulations(char *PortfolioObjectId,
                                        char *PathGenObjectId,
                                        OPER* dates,
                                        unsigned int numSims,
                                        OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {

        if (numSims <= 0)
        {
         ObjectHandler::scalarToOper("Number of sims must be poisitive", returnValue);
         return &returnValue;
        }
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("runPortfolioSimulations"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "Dates");
        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(DatesLib)) 
        {
         ObjectHandler::scalarToOper("Input dates vector not ordered", returnValue);
         return &returnValue;
        }

        OH_GET_OBJECT(PortfolioObjectIdPtr, PortfolioObjectId, sjdObjects::Portfolio)
        OH_GET_OBJECT(PathGenObjectIDPtr, PathGenObjectId, sjdObjects::SingleFactorGBMPathGenerator)
                
        PortfolioObjectIdPtr->runPortfolioSimulations(PathGenObjectIDPtr, DatesLib, numSims);

         if (!PortfolioObjectIdPtr->areValuationDatesOK())
         {
         ObjectHandler::scalarToOper("Unable to perform forward valuations", returnValue);
         return &returnValue;
       }

        ObjectHandler::scalarToOper(true, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getPortfolioMCAnalysis(char *PortfolioObjectId,
                                       OPER *valueDate,
                                       char* zeroDRSId,
                                       char* spreadDRSId,
                                       bool *returnColumnHeadings,
                                       char *cvaId,
                                       char *dvaId,
                                       OPER *Trigger) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;

    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getPortfolioMCAnalysis"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(ObjectIdLibObjPtr, PortfolioObjectId, sjdObjects::Portfolio)

        OH_GET_OBJECT(ZeroDRSIdLibObjPtr, zeroDRSId, sjdObjects::DiscountRateSource)
        if (!ZeroDRSIdLibObjPtr->isOK()) 
      {
         ObjectHandler::scalarToOper(ZeroDRSIdLibObjPtr->getErrorMessages(), returnValue);
         return &returnValue;
        }
        OH_GET_OBJECT(SpreadDRSIdLibObjPtr, spreadDRSId, sjdObjects::DiscountRateSource)
        if (!SpreadDRSIdLibObjPtr->isOK()) 
      {
         ObjectHandler::scalarToOper(SpreadDRSIdLibObjPtr->getErrorMessages(), returnValue);
         return &returnValue;
        }

        if (!ObjectIdLibObjPtr->getMCHasBeenRun())
      {
         ObjectHandler::scalarToOper("MC has not been run", returnValue);
         return &returnValue;
      }
            
        if (!ObjectIdLibObjPtr->areValuationDatesOK())
         {
         ObjectHandler::scalarToOper("Unable to perform forward valuations", returnValue);
         return &returnValue;
       }

         QuantLib::Date ValueDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*valueDate), "ValueDate");

        boost::shared_ptr<sjdObjects::CVA> CVAIdLibObjPtr, DVAIdLibObjPtr;
        try 
        {
            ObjectHandler::Repository::instance().retrieveObject(CVAIdLibObjPtr, cvaId);
        } 
      catch (...) {}
        try 
      {
            ObjectHandler::Repository::instance().retrieveObject(DVAIdLibObjPtr, dvaId);
        }  
      catch (...) {}

        
        std::vector<string> rowHeadings, columnHeadings;
        std::vector<std::vector<double> > data;
        std::vector<double> dataRow;
        bool ok = ObjectIdLibObjPtr->getPortfolioMCAnalysis(ValueDateLib, 
                                                            columnHeadings, 
                                                            dataRow, 
                                                            ZeroDRSIdLibObjPtr, 
                                                            SpreadDRSIdLibObjPtr, 
                                                            CVAIdLibObjPtr, 
                                                            DVAIdLibObjPtr);
        if (!ok)
      {
         ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessages(), returnValue);
         return &returnValue;
      }

        data.push_back(dataRow);
        if (!(*returnColumnHeadings))
      {
            columnHeadings.clear();
      }
        sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        return &returnValue;
   } 
   SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getPortfolioMCStatistics(char *PortfolioObjectId,
                                        bool *returnColumnHeadings,
                                        bool *returnDates,
                                        OPER *Trigger) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;

    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getPortfolioMCStatistics"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(ObjectIdLibObjPtr, PortfolioObjectId, sjdObjects::Portfolio)

        if (!ObjectIdLibObjPtr->getMCHasBeenRun())
        {
         ObjectHandler::scalarToOper("MC has not been run", returnValue);
         return &returnValue;
        }
        if (!ObjectIdLibObjPtr->areValuationDatesOK())
         {
         ObjectHandler::scalarToOper("Unable to perform forward valuations", returnValue);
         return &returnValue;
       }

        std::vector<string> rowHeadings, columnHeadings;
        std::vector<std::vector<double> > data;
        bool ok = ObjectIdLibObjPtr->getPortfolioMCStatistics(*returnColumnHeadings, *returnDates, rowHeadings, columnHeadings, data);
        if (!ok)
        {
         ObjectHandler::scalarToOper("Unable to perform analysis", returnValue);
         return &returnValue;
        }
        sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getPortfolioPathAnalysis(char *PortfolioObjectId,
                               OPER *valueDate,
                                         OPER *dates,
                                         OPER *spotPath,
                                         char* zeroDRSId,
                               char* spreadDRSId,
                                         bool* returnColumnHeadings,
                                         char *cvaId = NULL,
                                         char *dvaId = NULL,
                                         OPER *Trigger = NULL) 
{

   boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getPortfolioPathAnalysis"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(ZeroDRSIdLibObjPtr, zeroDRSId, sjdObjects::DiscountRateSource)
        if (!ZeroDRSIdLibObjPtr->isOK()) 
      {
         ObjectHandler::scalarToOper(ZeroDRSIdLibObjPtr->getErrorMessages(), returnValue);
         return &returnValue;
        }
        OH_GET_OBJECT(SpreadDRSIdLibObjPtr, spreadDRSId, sjdObjects::DiscountRateSource)
        if (!SpreadDRSIdLibObjPtr->isOK()) 
      {
         ObjectHandler::scalarToOper(SpreadDRSIdLibObjPtr->getErrorMessages(), returnValue);
         return &returnValue;
        }

        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "Dates");
        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(DatesLib)) 
        {
         ObjectHandler::scalarToOper("Input dates vector not ordered", returnValue);
         return &returnValue;
        }

        std::vector<double> path =
            ObjectHandler::operToVector<double>(*spotPath, "SpotRates");
        QuantLib::Date ValueDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*valueDate), "ValueDate");

        OH_GET_OBJECT(ObjectIdLibObjPtr, PortfolioObjectId, sjdObjects::Portfolio)
        if (ObjectIdLibObjPtr->getSimDatesForDates(DatesLib).size() != path.size()) 
        {
         ObjectHandler::scalarToOper("Date and Path inputs do not have the correct size. Is there a margin holding period?", returnValue);
         return &returnValue;
        }

        boost::shared_ptr<sjdObjects::CVA> CVAIdLibObjPtr, DVAIdLibObjPtr;
        try 
        {
            ObjectHandler::Repository::instance().retrieveObject(CVAIdLibObjPtr, cvaId);
        } catch (...) {}
        try 
        {
            ObjectHandler::Repository::instance().retrieveObject(DVAIdLibObjPtr, dvaId);
        }  catch (...) {}
            

        std::vector<string> rowHeadings, columnHeadings;
        std::vector<std::vector<double> > data;
        std::vector<double> dataRow;
        bool ok = ObjectIdLibObjPtr->getPortfolioPathAnalysis(ValueDateLib, 
                                                              DatesLib, 
                                                           path,  
                                                              columnHeadings, 
                                                              dataRow, 
                                                              ZeroDRSIdLibObjPtr, 
                                                              SpreadDRSIdLibObjPtr, 
                                                              CVAIdLibObjPtr, 
                                                              DVAIdLibObjPtr);
        if (!ok)
        {
         ObjectHandler::scalarToOper("Unable to perform analysis", returnValue);
         return &returnValue;
        }

        data.push_back(dataRow);
        if (!(*returnColumnHeadings))
        {
            columnHeadings.clear();
        }
        sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getPortfolioPathStatistics(char *PortfolioObjectId,
                                           OPER *dates,
                                           OPER *spotRates,
                                           bool *returnColumnHeadings,
                                           bool *returnDates,
                                           OPER *Trigger) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getPortfolioPathStatistics"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(ObjectIdLibObjPtr, PortfolioObjectId, sjdObjects::Portfolio)

        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "Dates");
        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(DatesLib)) 
        {
         ObjectHandler::scalarToOper("Input dates vector not ordered", returnValue);
         return &returnValue;
        }

        std::vector<double> path =
            ObjectHandler::operToVector<double>(*spotRates, "SpotRates");
        if (ObjectIdLibObjPtr->getSimDatesForDates(DatesLib).size() != path.size()) 
        {
         ObjectHandler::scalarToOper("Date and Path inputs do not have the correct size. Is there a margin holding period?", returnValue);
         return &returnValue;
        }

        std::vector<string> rowHeadings, columnHeadings;
        std::vector<std::vector<double> > data;
        bool ok = ObjectIdLibObjPtr->getPortfolioPathStatistics(*returnColumnHeadings, 
                                                                *returnDates, 
                                                                DatesLib, 
                                                                path, 
                                                                rowHeadings, 
                                                                columnHeadings, 
                                                                data);
        if (!ok)
        {
         ObjectHandler::scalarToOper("Unable to perform analysis", returnValue);
         return &returnValue;
        }

        if (!ObjectIdLibObjPtr->areValuationDatesOK())
        {
         ObjectHandler::scalarToOper("Unable to perform forward valuations", returnValue);
         return &returnValue;
      }
        sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}


DLLEXPORT OPER *getPortfolioRegCapAlongPath(char *ObjectId, OPER *Dates, OPER *spotRates, OPER *Trigger) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getPortfolioRegCapAlongPath"));
        ObjectHandler::validateRange(Trigger, "Trigger");
        // initialize the session ID (if enabled)
        SET_SESSION_ID

        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*Dates, "Dates");
        std::vector<double> rates =
            ObjectHandler::operToVector<double>(*spotRates, "SpotRates");
        std::vector<double> values = std::vector<double>(rates.size(), numeric_limits<double>::quiet_NaN());
        if (DatesLib.size() != rates.size())
        {
         ObjectHandler::scalarToOper("dates.size() != path.size()", returnValue);
         return &returnValue;
        }
        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(DatesLib)) 
        {
         ObjectHandler::scalarToOper("Input dates vector not ordered", returnValue);
         return &returnValue;
        }

        // convert object IDs into library objects
        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::PortfolioWithRegulatoryCapital)
        set<Date> datesSet(DatesLib.begin(), DatesLib.end());
        ObjectIdLibObjPtr->setForwardValuationDates(datesSet);
        
        // invoke the member function
        if (ObjectIdLibObjPtr->areValuationDatesOK())
        {
            values = ObjectIdLibObjPtr->getRegulatoryCapitalAlongPath(rates);
        }
        else 
         {
         ObjectHandler::scalarToOper("Unable to perform forward valuations", returnValue);
         return &returnValue;
       }
        ObjectHandler::vectorToOper(values, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}
*/

 