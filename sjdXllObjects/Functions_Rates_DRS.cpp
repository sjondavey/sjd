
#include <qlo/qladdindefines.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <sjd/rates/Discount/InterestRateConvention.h>
#include <sjd/rates/Discount/DiscountCurves.h>
#include <sjd/sjdObjects/RateSource/Discount/DiscountCurve.h>
#include <sjd/sjdObjects/RateSource/Discount/vo_DiscountCurve.h>

#include <boost/algorithm/string.hpp>

#include <sjd/sjdXllObjects/ObjectCommon.h>

/*======================================================================================
Create an instance of the object sjdObjects::DiscountCurve and return its ID
=======================================================================================*/
DLLEXPORT char *createDiscountFactorCurve(char *objectID,
                                          OPER *anchorDate,
                                          OPER *dates,
                                          OPER *df,
                                          OPER *interpolation,
                                          bool *permanent) 
{

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createDiscountFactorCurve"));

        // convert input datatypes to C++ datatypes

        ObjectHandler::property_t AnchorDateCpp = ObjectHandler::convert2<ObjectHandler::property_t>(
            ObjectHandler::ConvertOper(*anchorDate));

        std::vector<ObjectHandler::property_t> CurveDatesCpp =
            ObjectHandler::operToVector<ObjectHandler::property_t>(*dates, "CurveDates");

        std::vector<double> DiscountFactorsCpp =
            ObjectHandler::operToVector<double>(*df, "DiscountFactors");

        std::string InterpolationType = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*interpolation), "Interpolation");

        sjd::ArrayInterpolatorType sjdType = sjd::LINEAR;
        boost::to_lower(InterpolationType);
        boost::trim(InterpolationType);
        if (InterpolationType.compare("linear") == 0) 
        {
            sjdType = sjd::LINEAR;
        }
        else if (InterpolationType.compare("cubic") == 0) 
        {
            sjdType = sjd::CUBIC;
        }
        else 
        {
            ObjectHandler::stringToChar("Interpolator type must be either LINEAR or CUBIC", ret);
            return ret;
        }

        // convert input datatypes to QuantLib datatypes

        QuantLib::Date AnchorDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*anchorDate), "AnchorDate");

        std::vector<QuantLib::Date> CurveDatesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*dates, "CurveDates");

        // Strip the Excel cell update counter suffix from Object IDs
        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(objectID);

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::DiscountCurveValueObject(
                ObjectIdStrip,
                AnchorDateCpp,
                CurveDatesCpp,
                DiscountFactorsCpp,
                false,
                *permanent));

        sjdObjects::DiscountCurve *dc = new sjdObjects::DiscountCurve(
                valueObject,
                AnchorDateLib,
                CurveDatesLib,
                DiscountFactorsCpp,
                sjdType,
                false,
                *permanent);
        boost::shared_ptr<ObjectHandler::Object> object(dc);

        std::string returnValue;
        boost::shared_ptr<sjd::DiscountRateSource> sjdDrs;
        dc->getLibraryObject(sjdDrs);
        if (!sjdDrs->isOK()) 
        {
            returnValue = sjdDrs->getErrorMessagesAsString();
        }
        else 
        {
            // Store the Object in the Repository
            returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
        }
        // Convert and return the return value
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
   SJD_XLL_CATCH_STRING()
}


/*======================================================================================
Create an instance of the object sjdObjects::DiscountRateSource using a base curve and an
interest rate spread
=======================================================================================*/
DLLEXPORT char *createDRSByBumping(char *outputObjectID,
                                          char *inputObjectID,
                                          double spread,
                                          OPER *dayCounter, 
                                          OPER *compounding, 
                                          OPER *frequency,
                                          bool *permanent) 
{

    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createDRSByBumping"));

        if ((spread > 1) || (spread < 0))
        {
            ObjectHandler::stringToChar("Interest rate spread must be between 0 and 1", ret);
            return ret;
        }
        OH_GET_OBJECT(ObjectIdLibObjPtr, inputObjectID, sjdObjects::DeterministicDiscountRateSource)    
        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::stringToChar(ObjectIdLibObjPtr->getErrorMessages(), ret);
            return ret;
        }

        std::string DayCounterCpp = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*dayCounter), "DayCounter", "Actual/365 (Fixed)");
        QuantLib::DayCounter DayCounterEnum =
            ObjectHandler::Create<QuantLib::DayCounter>()(DayCounterCpp);

        boost::shared_ptr<QuantLib::DayCounter> dcp = boost::shared_ptr<QuantLib::DayCounter>(new QuantLib::DayCounter(DayCounterEnum));

        std::string CompoundingCpp = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*compounding), "Compounding", "Compounded"); // Simple, Compounded, Continuous
        QuantLib::Compounding CompoundingEnum =
            ObjectHandler::Create<QuantLib::Compounding>()(CompoundingCpp);

        std::string FrequencyCpp = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*frequency), "Frequency", "Annual"); // NoFrequency, Once, Annual, Semiannual, EveryFourthMonth, Quarterly, Bimonthly, Monthly
        QuantLib::Frequency FrequencyEnum =
            ObjectHandler::Create<QuantLib::Frequency>()(FrequencyCpp);

        
        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(outputObjectID);
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));

        boost::shared_ptr<sjdObjects::DiscountRateSource> dc = ObjectIdLibObjPtr->parallelBump(valueObject,
                                                                              spread,
                                                                              dcp,
                                                                              CompoundingEnum,
                                                                              FrequencyEnum,
                                                                              *permanent);
        boost::shared_ptr<ObjectHandler::Object> object(dc);

        std::string returnValue;
        boost::shared_ptr<sjd::DiscountRateSource> sjdDrs;
        dc->getLibraryObject(sjdDrs);
        if (!sjdDrs->isOK()) 
        {
            returnValue = sjdDrs->getErrorMessagesAsString();
        }
        else 
        {
            // Store the Object in the Repository
            returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
        }
        // Convert and return the return value
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    } 
   SJD_XLL_CATCH_STRING()
}



DLLEXPORT char *rollDRS(char *OutputDRSId, OPER *toDate, char *InputDRSID, bool *permanent) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static char ret[XL_MAX_STR_LEN];
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("rollDRS"));

        QuantLib::Date toDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*toDate), "ToDate");

        OH_GET_OBJECT(ObjectIdLibObjPtr, InputDRSID, sjdObjects::DeterministicDiscountRateSource)    
        if (!ObjectIdLibObjPtr->isInRange(toDateLib))
        {
            ObjectHandler::stringToChar("ToDate is not in DRS range", ret);        
            return ret;
        }
        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::stringToChar(ObjectIdLibObjPtr->getErrorMessages(), ret);        
            return ret;
        }

        std::string ObjectIdStrip = ObjectHandler::CallingRange::getStub(OutputDRSId);
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));


        boost::shared_ptr<sjdObjects::DiscountRateSource> dc = 
            ObjectIdLibObjPtr->rollForward(valueObject, toDateLib, *permanent);
        boost::shared_ptr<ObjectHandler::Object> object(dc);
        boost::shared_ptr<sjd::DiscountRateSource> sjdDrs;
        dc->getLibraryObject(sjdDrs);
        std::string returnValue;
        if (!sjdDrs->isOK()) 
        {
            returnValue = sjdDrs->getErrorMessagesAsString();
        }
        else 
        {
            returnValue =
                ObjectHandler::RepositoryXL::instance().storeObject(ObjectIdStrip, object, true);
        }
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;
    }
   SJD_XLL_CATCH_STRING()
}


/*======================================================================================
Any object which is derived from sjdObjects::DiscountRateSource can be queried
for its discount factors using this method
=======================================================================================*/
DLLEXPORT OPER *getDiscountFactor(char *ObjectId, OPER *XValues, OPER *Trigger) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("getDiscountFactor"));

        ObjectHandler::validateRange(Trigger, "Trigger");

        // convert input datatypes to QuantLib datatypes

        std::vector<QuantLib::Date> XValuesLib =
            ObjectHandler::operToVector<QuantLib::Date>(*XValues, "XValues");
        if (XValuesLib.size() == 0)
        {
            ObjectHandler::scalarToOper("No input date(s)", returnValue);        
            return &returnValue;
        }
        // convert input datatypes to Object references

        OH_GET_OBJECT(ObjectIdLibObjPtr, ObjectId, sjdObjects::DiscountRateSource)    
        if (!ObjectIdLibObjPtr->isInRange(XValuesLib.back()))
        {
            ObjectHandler::scalarToOper("Final Date not in interpolation range", returnValue);        
            return &returnValue;
        }

        if (!ObjectIdLibObjPtr->isInRange(XValuesLib.front())) 
        {
            ObjectHandler::scalarToOper("Initial Date not in interpolation range", returnValue);        
            return &returnValue;
        }

        if (!ObjectIdLibObjPtr->isOK())
        {
            ObjectHandler::scalarToOper(ObjectIdLibObjPtr->getErrorMessages(), returnValue);
        }
        else 
        {
            ObjectHandler::vectorToOper(ObjectIdLibObjPtr->getDiscountFactor(XValuesLib), returnValue);
        }
        return &returnValue;
    }
   SJD_XLL_CATCH_OPER()
}


/*======================================================================================
Convert a discount factor to an interest rate using the supplied convention
=======================================================================================*/
DLLEXPORT OPER *convertDFToRate(double df, 
                                OPER *fromDate,
                                OPER *toDate,
                                OPER *dayCounter, 
                                OPER *compounding, 
                                OPER *frequency) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("convertDFToRate"));

        if ((df > 1) || (df < 0))
        {
            ObjectHandler::scalarToOper("DF must be between 0 and 1", returnValue);        
            return &returnValue;
        }
        QuantLib::Date fromDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*fromDate), "ToDate");
        QuantLib::Date toDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*toDate), "ToDate");

        std::string DayCounterCpp = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*dayCounter), "DayCounter", "Actual/365 (Fixed)");
        QuantLib::DayCounter DayCounterEnum =
            ObjectHandler::Create<QuantLib::DayCounter>()(DayCounterCpp);

        boost::shared_ptr<QuantLib::DayCounter> dcp = boost::shared_ptr<QuantLib::DayCounter>(new QuantLib::DayCounter(DayCounterEnum));

        std::string CompoundingCpp = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*compounding), "Compounding", "Compounded"); // Simple, Compounded, Continuous
        QuantLib::Compounding CompoundingEnum =
            ObjectHandler::Create<QuantLib::Compounding>()(CompoundingCpp);

        std::string FrequencyCpp = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*frequency), "Frequency", "Annual"); // NoFrequency, Once, Annual, Semiannual, EveryFourthMonth, Quarterly, Bimonthly, Monthly
        QuantLib::Frequency FrequencyEnum =
            ObjectHandler::Create<QuantLib::Frequency>()(FrequencyCpp);


        //double rate = sjd::DiscountRateSource::dfToRate(df, fromDateLib, toDateLib, dcp, CompoundingEnum, FrequencyEnum);
        boost::shared_ptr<sjd::InterestRateConvention> irc = boost::shared_ptr<sjd::InterestRateConvention>(
            new sjd::InterestRateConvention(dcp, CompoundingEnum, FrequencyEnum));
        double rate = irc->dfToRate(df, fromDateLib, toDateLib);
        ObjectHandler::scalarToOper(rate, returnValue);        
        return &returnValue;

    }
   SJD_XLL_CATCH_OPER()
}

/*======================================================================================
Convert an interest rate to discount factor
=======================================================================================*/
DLLEXPORT OPER *convertRateToDF(double interestRate, 
                                OPER *fromDate,
                                OPER *toDate,
                                OPER *dayCounter, 
                                OPER *compounding, 
                                OPER *frequency) 
{
    // declare a shared pointer to the Function Call object
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        // instantiate the Function Call object
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>(
            new ObjectHandler::FunctionCall("convertRateToDF"));

        if ((interestRate > 1) || (interestRate < 0))
        {
            ObjectHandler::scalarToOper("InterestRate must be between 0 and 1", returnValue);        
            return &returnValue;
        }
        QuantLib::Date fromDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*fromDate), "ToDate");
        QuantLib::Date toDateLib = ObjectHandler::convert2<QuantLib::Date>(
            ObjectHandler::ConvertOper(*toDate), "ToDate");

        std::string DayCounterCpp = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*dayCounter), "DayCounter", "Actual/365 (Fixed)");
        QuantLib::DayCounter DayCounterEnum =
            ObjectHandler::Create<QuantLib::DayCounter>()(DayCounterCpp);

        boost::shared_ptr<QuantLib::DayCounter> dcp = boost::shared_ptr<QuantLib::DayCounter>(new QuantLib::DayCounter(DayCounterEnum));

        std::string CompoundingCpp = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*compounding), "Compounding", "Compounded"); // Simple, Compounded, Continuous
        QuantLib::Compounding CompoundingEnum =
            ObjectHandler::Create<QuantLib::Compounding>()(CompoundingCpp);

        std::string FrequencyCpp = ObjectHandler::convert2<std::string>(
            ObjectHandler::ConvertOper(*frequency), "Frequency", "Annual"); // NoFrequency, Once, Annual, Semiannual, EveryFourthMonth, Quarterly, Bimonthly, Monthly
        QuantLib::Frequency FrequencyEnum =
            ObjectHandler::Create<QuantLib::Frequency>()(FrequencyCpp);


        //double rate = sjd::DiscountRateSource::rateToDF(interestRate, fromDateLib, toDateLib, dcp, CompoundingEnum, FrequencyEnum);
        boost::shared_ptr<sjd::InterestRateConvention> irc = boost::shared_ptr<sjd::InterestRateConvention>(
            new sjd::InterestRateConvention(dcp, CompoundingEnum, FrequencyEnum));
        double rate = irc->rateToDF(interestRate, fromDateLib, toDateLib);

        ObjectHandler::scalarToOper(rate, returnValue);        
        return &returnValue;

    }
   SJD_XLL_CATCH_OPER()
}


