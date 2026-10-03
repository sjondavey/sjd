
#include <qlo/qladdindefines.hpp>
#include <sjd/sjdObjects/Contracts/ContractStressor.h>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <Tools/vo_GenericUnimplemented.h>
#include <sjd/sjdXllObjects/ObjectCommon.h>

#include <Tools/vectorOfVectorsToOper.h>

#define XLL_DEC DLLEXPORT

DLLEXPORT OPER *createMarketRiskReport(char *inputContract, char *ratesEnvironment) 
{
    boost::shared_ptr<ObjectHandler::FunctionCall> functionCall;
    static XLOPER returnValue;
    try 
    {
        functionCall = boost::shared_ptr<ObjectHandler::FunctionCall>
            (new ObjectHandler::FunctionCall("createMarketRiskReport"));

        // convert input datatypes to QuantLib datatypes

        OH_GET_OBJECT(ContractIdLibObjPtr, inputContract, sjdObjects::Contract)
        OH_GET_OBJECT(RatesIdLibObjPtr, ratesEnvironment, sjdObjects::RatesEnvironment)

        // Strip the Excel cell update counter suffix from Object IDs
        
        std::string ObjectIdStrip = "tmpMktRiskReport";

        // Construct the Value Object
        boost::shared_ptr<ObjectHandler::ValueObject> valueObject(
            new sjdObjects::ValueObjects::GenericUnimplementedValueObject(ObjectIdStrip,false));
        
        boost::shared_ptr<sjdObjects::ContractStressor> cs(
            new sjdObjects::ContractStressor(valueObject, ContractIdLibObjPtr, RatesIdLibObjPtr, false));

        if (!cs->isOK()) 
        {
            string msg = cs->getErrorMessages();
            unsigned maxStringSizeInExcel = 254;
            if (msg.size() > maxStringSizeInExcel) 
            {
                msg = msg.substr(0,maxStringSizeInExcel-1);
            }
            ObjectHandler::scalarToOper(msg, returnValue);
        }
        else 
        {
            std::vector<pair<string, double> > strssedValues = cs->getStressedValues();
            std::vector<string> columnHeadings;
            columnHeadings.push_back("MtM");
            std::vector<string> rowHeadings;
            std::vector<std::vector<double> > data;
            for (size_t i = 0; i < strssedValues.size(); ++i) 
            {
                rowHeadings.push_back(strssedValues[i].first);
                std::vector<double> mtm;
                mtm.push_back(strssedValues[i].second);
                data.push_back(mtm);
            }
            sjd::tableToOper(columnHeadings, rowHeadings, data, returnValue);
        }
        return &returnValue;
    } 
    SJD_XLL_CATCH_OPER()
}

