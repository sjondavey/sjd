#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>


#include <sjd/sjdObjects/RateSource/PathGenerators/SingleFactorGMB.h>
#include <sjd/sjdObjects/RateSource/Forward/ForwardRateSource.h>
#include <sjd/sjdObjects/RateSource/Volatility/VolatilityRateSource.h>
#include <Tools/vo_GenericUnimplemented.h>

#include <sjd/maths/maths.h>

#include <sjd/sjdXllObjects/ObjectCommon.h>


DLLEXPORT char *createESGSingleFactor(char *objectID,
                                      char *rateSourceId,
                                      bool *permanent) 
{

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try {

        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createESGSingleFactor"));


        OH_GET_OBJECT(rePtr, rateSourceId, sjdObjects::RatesEnvironment)

        // Strip the Excel cell update counter suffix from Object IDs        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(
                ObjectIdStrip, *permanent));

        sjdObjects::SingleFactorEconomicScenarioGenerator* esg = new 
            sjdObjects::SingleFactorEconomicScenarioGenerator(valueObject, rePtr, *permanent);
        if (!esg->isOK())
        {
            ObjectHandler::stringToChar(esg->getErrorMessage(), ret);
            return ret;
        }
        boost::shared_ptr<ObjectHandler::Object> object(esg);

        // Store the Object in the Repository

        std::string returnValue =
            ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;

    } 
    SJD_XLL_CATCH_STRING()
}


DLLEXPORT OPER *getESGPathForCI(char *ObjectId,
                                OPER *dates,
                                double confidenceInterval,
                                OPER *Trigger) 
{

    // declare a shared pointer to the Function Call object

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
                new ObjectHandler::FunctionCall("getESGPathForCI"));

        ObjectHandler::validateRange(Trigger, "Trigger");

        SET_SESSION_ID

        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "Dates");
        
        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(DatesLib)) 
        {
            ObjectHandler::scalarToOper("Input dates vector not increasing", returnValue);        
            return &returnValue;
        }
        set<Date> datesSet = set<Date>(DatesLib.begin(), DatesLib.end());

        // convert object IDs into library objects

          OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::SingleFactorEconomicScenarioGenerator)        
        ObjectIdLibObjPtr->setSimulationDates(datesSet);        
        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessage(), returnValue);
            return &returnValue;
        }
        vector<boost::shared_ptr<sjd::RatesEnvironment>> reVector = ObjectIdLibObjPtr->getPathForCI(confidenceInterval);
        vector<string> objectNames = vector<string>(reVector.size());

        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(ObjectId);
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, false));

        for (size_t i = 0; i < reVector.size(); ++i)
        {
            stringstream objDescription;
            objDescription << ObjectIdStrip << "_ci_" << (QuantLib::io::iso_date(DatesLib[i]));

            boost::shared_ptr<sjdObjects::RatesEnvironment> sjdRE = boost::shared_ptr<sjdObjects::RatesEnvironment>(
                new sjdObjects::RatesEnvironment(valueObject, reVector[i], false));

            boost::shared_ptr<ObjectHandler::Object> object(sjdRE);
            std::string returnValue;
            if (!sjdRE->isOK()) 
            {
                objectNames[i] = sjdRE->getErrorMessages();
            }
            else 
            {
                objectNames[i] =
                    ObjectHandler::RepositoryXL::instance().storeObject(objDescription.str(), object, true);
            }
        }
        ObjectHandler::vectorToOper(objectNames, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()

}


DLLEXPORT OPER *getESGPathForSpot(char *ObjectId,
                                 OPER *dates,
                                 OPER *spotPath,
                                 OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;

    try 
    {
        // instantiate the Function Call object

        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getESGPathForSpot"));

        ObjectHandler::validateRange(Trigger, "Trigger");

        SET_SESSION_ID

        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "Dates");
        
        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(DatesLib)) 
        {
            ObjectHandler::scalarToOper("Input dates vector not increasing", returnValue);        
            return &returnValue;
        }
        set<Date> datesSet = set<Date>(DatesLib.begin(), DatesLib.end());

        std::vector<double> spotPathLib =
            ObjectHandler::operToVector<double>(*spotPath, "Spot Path");
        if (spotPathLib.size() != DatesLib.size()) 
        {
            ObjectHandler::scalarToOper("Dates input is not the same size as the spot path", returnValue);        
            return &returnValue;
        }

          OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::SingleFactorEconomicScenarioGenerator)        
        ObjectIdLibObjPtr->setSimulationDates(datesSet);        
        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessage(), returnValue);
            return &returnValue;
        }
        vector<boost::shared_ptr<sjd::RatesEnvironment>> reVector = ObjectIdLibObjPtr->getPathForSpot(spotPathLib);
        vector<string> objectNames = vector<string>(reVector.size());

        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(ObjectId);
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip, false));

        for (size_t i = 0; i < reVector.size(); ++i)
        {
            stringstream objDescription;
            objDescription << ObjectIdStrip << "_spot_" << (QuantLib::io::iso_date(DatesLib[i]));

            boost::shared_ptr<sjdObjects::RatesEnvironment> sjdRE = boost::shared_ptr<sjdObjects::RatesEnvironment>(
                new sjdObjects::RatesEnvironment(valueObject, reVector[i], false));

            boost::shared_ptr<ObjectHandler::Object> object(sjdRE);
            std::string returnValue;
            if (!sjdRE->isOK()) 
            {
                objectNames[i] = sjdRE->getErrorMessages();
            }
            else 
            {
                objectNames[i] =
                    ObjectHandler::RepositoryXL::instance().storeObject(objDescription.str(), object, true);
            }
        }
        ObjectHandler::vectorToOper(objectNames, returnValue);
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()

        /*
        std::vector<QuantLib::Date> DatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "Dates");

        if (!sjd::isStrictlyIncreasing<QuantLib::Date>(DatesLib)) 
        {
            ObjectHandler::scalarToOper("Input dates vector not increasing", returnValue);        
            return &returnValue;
        }
        set<Date> datesSet = set<Date>(DatesLib.begin(), DatesLib.end());

        // convert object IDs into library objects

        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::SingleFactorGBMPathGenerator)        
        ObjectIdLibObjPtr->setSimulationDates(datesSet);        

        vector<double> values;
        // invoke the member function
        if (ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::vectorToOper(ObjectIdLibObjPtr->getRandomPath(), returnValue);
        }
        else 
        {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessage(), returnValue);
        }
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
    */
}


