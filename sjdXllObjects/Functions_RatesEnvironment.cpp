#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <sjd/sjdObjects/RateSource/RatesEnvironment.h>
#include <Tools/vo_GenericUnimplemented.h>
#include <sjd/sjdXllObjects/ObjectCommon.h>


DLLEXPORT char *createRatesEnvironment(char *objectID,
                                       OPER *anchorDate,
                                       char *drs,
                                       char *frs,
                                       char *vrs,
                                       char *fixingRS,
                                       char *foreigndrs,
                                       bool *permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createRatesEnvironment"));

        // convert input datatypes to C++ datatypes

        QuantLib::Date AnchorDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*anchorDate), "AnchorDate");

        boost::shared_ptr<sjdObjects::DiscountRateSource> drsObjectIdLibObjPtr = 
            boost::shared_ptr<sjdObjects::DiscountRateSource>();
        if (strlen(drs) > 0)
        {
            ObjectHandler::Repository::instance().retrieveObject(drsObjectIdLibObjPtr, drs);
        }
        
        boost::shared_ptr<sjdObjects::DiscountRateSource> foreignDrsObjectIdLibObjPtr = 
            boost::shared_ptr<sjdObjects::DiscountRateSource>();
        if (strlen(foreigndrs) > 0)
        {
            ObjectHandler::Repository::instance().retrieveObject(foreignDrsObjectIdLibObjPtr, foreigndrs);
        }

        boost::shared_ptr<sjdObjects::ForwardRateSource> frsObjectIdLibObjPtr = 
            boost::shared_ptr<sjdObjects::ForwardRateSource>();
        if (strlen(frs) > 0)
        {
            ObjectHandler::Repository::instance().retrieveObject(frsObjectIdLibObjPtr, frs);
        }

        boost::shared_ptr<sjdObjects::VolatilityRateSource> vrsObjectIdLibObjPtr = 
            boost::shared_ptr<sjdObjects::VolatilityRateSource>();
        if (strlen(vrs) > 0)
        {
            ObjectHandler::Repository::instance().retrieveObject(vrsObjectIdLibObjPtr, vrs);
        }

        boost::shared_ptr<sjdObjects::FixingRateSource> fixingrsObjectIdLibObjPtr = 
            boost::shared_ptr<sjdObjects::FixingRateSource>();
        if (strlen(fixingRS) > 0)
        {
            ObjectHandler::Repository::instance().retrieveObject(fixingrsObjectIdLibObjPtr, fixingRS);
        }


        // Strip the Excel cell update counter suffix from Object IDs
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        // Construct the Object
        
        sjdObjects::RatesEnvironment *re = new sjdObjects::RatesEnvironment(valueObject, 
                                                                            AnchorDateLib,
                                                                            drsObjectIdLibObjPtr, 
                                                                            foreignDrsObjectIdLibObjPtr, 
                                                                            frsObjectIdLibObjPtr, 
                                                                            vrsObjectIdLibObjPtr, 
                                                                            fixingrsObjectIdLibObjPtr, 
                                                                            *permanent);

        boost::shared_ptr<ObjectHandler::Object> object(re);
        if (!re->isOK()) 
        {
            ObjectHandler::stringToChar(re->getErrorMessages(), ret);
        }
        else 
        {
            // Store the Object in the Repository
            std::string returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
            ObjectHandler::stringToChar(returnValue, ret);
        }
        return ret;
    } 
    SJD_XLL_CATCH_STRING()
}

DLLEXPORT char *getRatesEnvironmentDRS(char *reID,
                                       OPER *Trigger,
                                       bool* permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("getRatesEnvironmentDRS"));

        OH_GET_OBJECT(reLibObjPtr, reID, sjdObjects::RatesEnvironment)    

        boost::shared_ptr<sjd::DiscountRateSource> sjdDRS;
        if (reLibObjPtr->hasDRS())
        {
            sjdDRS = reLibObjPtr->getDRS();
        }
        else 
        {
            ObjectHandler::stringToChar("Rates environment does not contain a drs", ret);
            return ret;
        }
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(reID);
        ObjectIdStrip += "_DRS";


        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        sjdObjects::DiscountRateSource *drs = new sjdObjects::DiscountRateSource(valueObject, sjdDRS, *permanent);

        boost::shared_ptr<ObjectHandler::Object> object(drs);
        if (!drs->isOK()) 
        {
            ObjectHandler::stringToChar(drs->getErrorMessages(), ret);
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

DLLEXPORT char *getRatesEnvironmentForeignDRS(char *reID,
                                              OPER *Trigger,
                                              bool* permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("getRatesEnvironmentForeignDRS"));

        OH_GET_OBJECT(reLibObjPtr, reID, sjdObjects::RatesEnvironment)    

        boost::shared_ptr<sjd::DiscountRateSource> sjdForeignDRS;
        if (reLibObjPtr->hasForeignDRS())
        {
            sjdForeignDRS = reLibObjPtr->getForeignDRS();
        }
        else 
        {
            ObjectHandler::stringToChar("Rates environment does not contain a foreign DRS", ret);
            return ret;
        }
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(reID);
        ObjectIdStrip += "_ForeignDRS";


        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        sjdObjects::DiscountRateSource *drs = new sjdObjects::DiscountRateSource(valueObject, sjdForeignDRS, *permanent);

        boost::shared_ptr<ObjectHandler::Object> object(drs);
        if (!drs->isOK()) 
        {
            ObjectHandler::stringToChar(drs->getErrorMessages(), ret);
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


DLLEXPORT char *getRatesEnvironmentFRS(char *reID,
                                       OPER *Trigger,
                                       bool* permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("getRatesEnvironmentFRS"));

        OH_GET_OBJECT(reLibObjPtr, reID, sjdObjects::RatesEnvironment)    

        boost::shared_ptr<sjd::ForwardRateSource> sjdFRS;
        if (reLibObjPtr->hasFRS())
        {
            sjdFRS = reLibObjPtr->getFRS();
        }
        else 
        {
            ObjectHandler::stringToChar("Rates environment does not contain a frs", ret);
            return ret;
        }
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(reID);
        ObjectIdStrip += "_FRS";

        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        sjdObjects::ForwardRateSource *frs = new sjdObjects::ForwardRateSource(valueObject, sjdFRS, *permanent);

        boost::shared_ptr<ObjectHandler::Object> object(frs);
        if (!frs->isOK()) 
        {
            ObjectHandler::stringToChar(frs->getErrorMessages(), ret);
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

DLLEXPORT char *getRatesEnvironmentVRS(char *reID,
                                       OPER *Trigger,
                                       bool* permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("getRatesEnvironmentVRS"));

        OH_GET_OBJECT(reLibObjPtr, reID, sjdObjects::RatesEnvironment)    

        boost::shared_ptr<sjd::VolatilityRateSource> sjdVRS;
        if (reLibObjPtr->hasVRS())
        {
            sjdVRS = reLibObjPtr->getVRS();
        }
        else 
        {
            ObjectHandler::stringToChar("Rates environment does not contain a vrs", ret);
            return ret;
        }
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(reID);
        ObjectIdStrip += "_VRS";

        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        sjdObjects::VolatilityRateSource *vrs = new sjdObjects::VolatilityRateSource(valueObject, sjdVRS, *permanent);

        boost::shared_ptr<ObjectHandler::Object> object(vrs);
        if (!vrs->isOK()) 
        {
            ObjectHandler::stringToChar(vrs->getErrorMessages(), ret);
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

DLLEXPORT char *getRatesEnvironmentFixingRS(char *reID,
                                            OPER *Trigger,
                                            bool* permanent) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("getRatesEnvironmentFixingRS"));

        OH_GET_OBJECT(reLibObjPtr, reID, sjdObjects::RatesEnvironment)    

        boost::shared_ptr<sjd::RollingFixingRateSource> sjdFixingRS;
        if (reLibObjPtr->hasFixingRS())
        {
            boost::shared_ptr<sjd::FixingRateSource> sjdFRS = reLibObjPtr->getFixingRS();
            sjdFixingRS = boost::dynamic_pointer_cast<sjd::RollingFixingRateSource>(sjdFRS);
            if (sjdFixingRS == 0)
            {
                ObjectHandler::stringToChar("Rates environment does not contain a rolling fixingrs", ret);
                return ret;
            }
        }
        else 
        {
            ObjectHandler::stringToChar("Rates environment does not contain a fixingrs", ret);
            return ret;
        }
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(reID);
        ObjectIdStrip += "_FixingRS";


        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        sjdObjects::FixingRateSource *fixingrs = new sjdObjects::FixingRateSource(valueObject, sjdFixingRS, *permanent);

        boost::shared_ptr<ObjectHandler::Object> object(fixingrs);
        if (!fixingrs->isOK()) 
        {
            ObjectHandler::stringToChar(fixingrs->getErrorMessages(), ret);
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
