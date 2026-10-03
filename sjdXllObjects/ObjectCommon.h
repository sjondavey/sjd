#ifndef SJD_OBJ_COMMON
#define SJD_OBJ_COMMON


#include <oh/libraryobject.hpp>
#include <sjd/tools/Common.h>
#include <boost/algorithm/string.hpp>


using namespace std;

namespace sjdObjects 
{

   const string error = "unkown error type";

   
//! Get a boost shared pointer to a class derived from Object.
#define SJD_XLL_CATCH_OPER() \
   catch (const std::exception &e) \
   {\
        ObjectHandler::RepositoryXL::instance().logError(e.what(), functionCall);\
        ObjectHandler::scalarToOper(e.what(), returnValue);\
        return &returnValue;\
    } \
   catch (...) \
   {\
        ObjectHandler::RepositoryXL::instance().logError(sjdObjects::error, functionCall);\
        ObjectHandler::scalarToOper(sjdObjects::error, returnValue);\
        return &returnValue;\
    };



#define SJD_XLL_CATCH_STRING() \
   catch (const std::exception &e) \
   {\
        ObjectHandler::RepositoryXL::instance().logError(e.what(), functionCall);\
        ObjectHandler::stringToChar(e.what(), ret);\
        return ret;\
    }\
    catch (...) \
    {\
        ObjectHandler::RepositoryXL::instance().logError(sjdObjects::error, functionCall);\
        ObjectHandler::stringToChar(sjdObjects::error, ret);\
        return ret;\
    };

   bool convertBuySellInput(char* buySell, sjd::BuySell &bs);

   bool convertPutCallInput(char* putCall, sjd::PutCall &pc);

}

#endif
