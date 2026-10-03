#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <Tools/vo_GenericUnimplemented.h>
#include <sjd/sjdObjects/Contracts/Portfolio/PortfolioEvolver.h>

#include <Tools/vectorOfVectorsToOper.h>
#include <sjd/sjdXllObjects/ObjectCommon.h>


DLLEXPORT char *createPortfolioEvolver(char *objectID, char *portfolioID, char *pathGeneratorID, OPER* dates, int holdingPeriodInDays, bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createPortfolioEvolver"));


        OH_GET_OBJECT(portfolioIDLibObjPtr, portfolioID, sjdObjects::Portfolio);
        OH_GET_OBJECT(pathGeneratorIDLibObjPtr, pathGeneratorID, sjdObjects::SingleFactorEconomicScenarioGenerator);
        
        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "Dates");        
        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(DatesLib)) 
        {
            ObjectHandler::stringToChar("Input dates vector not increasing", ret);
            return ret;
        }
        set<Date> dateSet = set<Date>(DatesLib.begin(), DatesLib.end());


        // Strip the Excel cell update counter suffix from Object IDs
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,*permanent));
        
        sjdObjects::PortfolioEvolver *pe;
        
        if (holdingPeriodInDays <= 0)
        {
            pe = new sjdObjects::PortfolioEvolver(valueObject, 
                                                  portfolioIDLibObjPtr, 
                                                  pathGeneratorIDLibObjPtr, 
                                                  dateSet, 
                                                  *permanent);        
        }
        else
        {
            pe = new sjdObjects::PortfolioEvolver(valueObject, 
                                                  portfolioIDLibObjPtr, 
                                                  pathGeneratorIDLibObjPtr, 
                                                  dateSet, 
                                                  holdingPeriodInDays,
                                                  *permanent);        
        }
        if (!pe->isOK())
        {
            ObjectHandler::stringToChar(pe->getErrorMessages(), ret);
            return ret;
        }
        boost::shared_ptr<ObjectHandler::Object> object = boost::shared_ptr<ObjectHandler::Object>(pe);
        std::string returnValue =
            ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}

DLLEXPORT OPER *runPortfolioEvolverSimulations(char *portfolioEvolverId,
                                               unsigned int numSims,
                                               OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("runPortfolioEvolverSimulations"));
        ObjectHandler::validateRange(Trigger, "Trigger");
        if (numSims <= 0)
        {
            ObjectHandler::scalarToOper("Number of sims must be poisitive", returnValue);
            return &returnValue;
        }

        OH_GET_OBJECT(portfolioEvolverIdPtr, portfolioEvolverId, sjdObjects::PortfolioEvolver)
                
        portfolioEvolverIdPtr->runMonteCarlo(numSims);

        ObjectHandler::scalarToOper(true, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getMCDetail(char *portfolioEvolverId,
                            OPER *Trigger) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getMCDetail"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(portfolioEvolverIdPtr, portfolioEvolverId, sjdObjects::PortfolioEvolver)

        if (!portfolioEvolverIdPtr->hasMCBeenRun())
        {
            ObjectHandler::scalarToOper("MC has not been run", returnValue);
            return &returnValue;
        }

        std::vector<string> columnHeadings;
        std::vector<std::vector<double> > data;
        std::vector<Date> simulationDates;
        portfolioEvolverIdPtr->getMonteCarloDetail(simulationDates, columnHeadings, data);

        std::vector<string> rowHeadings = std::vector<string>(0);
        for (size_t i = 0; i < simulationDates.size(); ++i)
        {
            stringstream errorDescription;
            errorDescription << QuantLib::io::iso_date(simulationDates[i]);
            rowHeadings.push_back(errorDescription.str());
        }

        sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getMCAnalysis(char *portfolioEvolverId,
                              char* zeroDRSId,
                              char* spreadDRSId,
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
            new ObjectHandler::FunctionCall("getMCAnalysis"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(portfolioEvolverIdPtr, portfolioEvolverId, sjdObjects::PortfolioEvolver)
        OH_GET_OBJECT(ZeroDRSIdLibObjPtr, zeroDRSId, sjdObjects::DiscountRateSource)
        OH_GET_OBJECT(SpreadDRSIdLibObjPtr, spreadDRSId, sjdObjects::DiscountRateSource)

        if (!portfolioEvolverIdPtr->hasMCBeenRun())
        {
            ObjectHandler::scalarToOper("MC has not been run", returnValue);
            return &returnValue;
        }
            
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
        portfolioEvolverIdPtr->getMonteCarloAnalysis(columnHeadings, 
                                                    dataRow, 
                                                    ZeroDRSIdLibObjPtr, 
                                                    SpreadDRSIdLibObjPtr, 
                                                    CVAIdLibObjPtr, 
                                                    DVAIdLibObjPtr);

        data.push_back(dataRow);
        sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        return &returnValue;
   } 
   SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getCIDetail(char *portfolioEvolverId,
                            double ci,
                            OPER *Trigger) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getCIDetail"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(portfolioEvolverIdPtr, portfolioEvolverId, sjdObjects::PortfolioEvolver)

        if ((ci < 1e-14) || (ci > (1 - 1e-14)))
        {
            ObjectHandler::scalarToOper("Confidence interval must be in the range (0,1)", returnValue);
            return &returnValue;
        }

        std::vector<string> columnHeadings;
        std::vector<std::vector<double> > data;
        std::vector<Date> simulationDates;
        portfolioEvolverIdPtr->getCIDetail(ci, simulationDates, columnHeadings, data);
        std::vector<string> rowHeadings = std::vector<string>(0);
        for (size_t i = 0; i < simulationDates.size(); ++i)
        {
            stringstream errorDescription;
            errorDescription << QuantLib::io::iso_date(simulationDates[i]);
            rowHeadings.push_back(errorDescription.str());
        }

        sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getCIAnalysis(char *portfolioEvolverId,
                              double ci,
                              char* zeroDRSId,
                              char* spreadDRSId,
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
            new ObjectHandler::FunctionCall("getCIAnalysis"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(portfolioEvolverIdPtr, portfolioEvolverId, sjdObjects::PortfolioEvolver)
        OH_GET_OBJECT(ZeroDRSIdLibObjPtr, zeroDRSId, sjdObjects::DiscountRateSource)
        OH_GET_OBJECT(SpreadDRSIdLibObjPtr, spreadDRSId, sjdObjects::DiscountRateSource)

        if ((ci < 1e-14) || (ci > (1 - 1e-14)))
        {
            ObjectHandler::scalarToOper("Confidence interval must be in the range (0,1)", returnValue);
            return &returnValue;
        }
            
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
        portfolioEvolverIdPtr->getCIAnalysis(ci, 
                                             columnHeadings, 
                                             dataRow, 
                                             ZeroDRSIdLibObjPtr, 
                                             SpreadDRSIdLibObjPtr, 
                                             CVAIdLibObjPtr, 
                                             DVAIdLibObjPtr);

        data.push_back(dataRow);
        sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        return &returnValue;
   } 
   SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getSpotPathDetail(char *portfolioEvolverId,
                                  OPER *spotPath,
                                  OPER *Trigger) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getSpotPathDetail"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(portfolioEvolverIdPtr, portfolioEvolverId, sjdObjects::PortfolioEvolver)

        std::vector<double> spotPathCpp =
            ObjectHandler::operToVector<double>(*spotPath, "Spot Path");
        size_t numSims = portfolioEvolverIdPtr->getSimulationDates().size();
        if (spotPathCpp.size() != numSims)
        {
            ObjectHandler::scalarToOper("spot path does not have the correct size", returnValue);
            return &returnValue;
        }

        std::vector<string> columnHeadings;
        std::vector<std::vector<double> > data;
        std::vector<Date> simulationDates;
        portfolioEvolverIdPtr->getSpotPathDetail(spotPathCpp, simulationDates, columnHeadings, data);
        std::vector<string> rowHeadings = std::vector<string>(0);
        for (size_t i = 0; i < simulationDates.size(); ++i)
        {
            stringstream errorDescription;
            errorDescription << QuantLib::io::iso_date(simulationDates[i]);
            rowHeadings.push_back(errorDescription.str());
        }

        sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

DLLEXPORT OPER *getSpotPathAnalysis(char *portfolioEvolverId,
                                    OPER *spotPath,
                                    char* zeroDRSId,
                                    char* spreadDRSId,
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
            new ObjectHandler::FunctionCall("getSpotPathAnalysis"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(portfolioEvolverIdPtr, portfolioEvolverId, sjdObjects::PortfolioEvolver)
        OH_GET_OBJECT(ZeroDRSIdLibObjPtr, zeroDRSId, sjdObjects::DiscountRateSource)
        OH_GET_OBJECT(SpreadDRSIdLibObjPtr, spreadDRSId, sjdObjects::DiscountRateSource)

        std::vector<double> spotPathCpp =
            ObjectHandler::operToVector<double>(*spotPath, "Spot Path");
        size_t numSims = portfolioEvolverIdPtr->getSimulationDates().size();
        if (spotPathCpp.size() != numSims)
        {
            ObjectHandler::scalarToOper("spot path does not have the correct size", returnValue);
            return &returnValue;
        }
            
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
        portfolioEvolverIdPtr->getSpotPathAnalysis(spotPathCpp, 
                                             columnHeadings, 
                                             dataRow, 
                                             ZeroDRSIdLibObjPtr, 
                                             SpreadDRSIdLibObjPtr, 
                                             CVAIdLibObjPtr, 
                                             DVAIdLibObjPtr);

        data.push_back(dataRow);
        sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        return &returnValue;
   } 
   SJD_XLL_CATCH_OPER()
}




DLLEXPORT OPER *getSimulationDatesForEvolver(char *portfolioEvolverId,                                   
                                             OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getSimulationDatesForEvolver"));
        ObjectHandler::validateRange(Trigger, "Trigger");

        OH_GET_OBJECT(portfolioEvolverIdPtr, portfolioEvolverId, sjdObjects::PortfolioEvolver)

        std::vector<Date> simDates = portfolioEvolverIdPtr->getSimulationDates();

        ObjectHandler::vectorToOper(simDates, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}



 