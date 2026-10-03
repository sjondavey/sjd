#include <ohxl/objecthandlerxl.hpp>
#include <qlo/qladdindefines.hpp>
#include <qlo/enumerations/register/register_all.hpp>
#include <qlo/serialization/serializationfactory.hpp>
#include <sjd/sjdXllObjects/Register_All.h>

#if defined BOOST_MSVC       // Microsoft Visual C++
#  define BOOST_LIB_DIAGNOSTIC
#include <qlo/auto_link.hpp>
#include <oh/auto_link.hpp>
#include <ql/auto_link.hpp>
#  if defined(XLL_STATIC)
     #include <ohxl/register/register_all.hpp>
     #include <ohxl/functions/export.hpp>
     #pragma message("XLL_STATIC is defined")
#  elif defined(XLL_IMPORTS)
     #include <xlsdk/auto_link.hpp>
     #pragma message("XLL_IMPORTS is defined")
#  endif
#  undef BOOST_LIB_DIAGNOSTIC
#endif

#if defined COMPILING_XLL_DYNAMIC
#   pragma message("COMPILING_XLL_DYNAMIC is defined")
#else
#   pragma message("COMPILING_XLL_DYNAMIC is NOT defined")
#endif

void init() {
   #ifdef XLL_STATIC
       // Instantiate the ObjectHandler Repository
       static ObjectHandler::RepositoryXL repositoryXL;
       // Instantiate the Enumerated Type Registry
       static ObjectHandler::EnumTypeRegistry enumTypeRegistry;
       // Instantiate the Enumerated Class Registry
       static ObjectHandler::EnumClassRegistry enumClassRegistry;
       // Instantiate the Enumerated Pair Registry
       static ObjectHandler::EnumPairRegistry enumPairRegistry;
   #endif
    // Instantiate the Processor Factory
    static ObjectHandler::ProcessorFactory processorFactory;
   // Instantiate the Serialization Factory
   // static QuantLibAddin::SerializationFactory factory;
}

DLLEXPORT void xlAutoFree(XLOPER *px) {
    freeOper(px);
}

DLLEXPORT XLOPER *xlAddInManagerInfo(XLOPER *xlAction) {

   init();

    XLOPER xlReturn;
    static XLOPER xlLongName;

    // Coerce the argument XLOPER to an integer.
    Excel(xlCoerce, &xlReturn, 2, xlAction, TempInt(xltypeInt));

    // The only valid argument value is 1. In this case we return the
    // long name for the XLL. Any other value should result in the
    // return of a #VALUE! error.
    if (1 == xlReturn.val.w) {
        ObjectHandler::scalarToOper(std::string("sjdObjects"), xlLongName);
    } else {
        xlLongName.xltype = xltypeErr;
        xlLongName.val.err = xlerrValue;
    }

    return &xlLongName;
}



DLLEXPORT int xlAutoOpen() {

   init();

    XLOPER xDll;

    try {

        Excel(xlGetName, &xDll, 0);

#ifdef XLL_STATIC
        // Initialize configuration info
        ObjectHandler::Configuration::instance().init();

        // Initialize ObjectHandler functions
        registerOhFunctions(xDll);
#endif
        // Initialize QuantLib functions
        registerSJDFunctions(xDll);

        // Initialize the Enumeration Registry
        QuantLibAddin::registerEnumerations();



        Excel(xlFree, 0, 1, &xDll);
        return 1;
    } catch (const std::exception &e) {
        std::ostringstream err;
        err << "Error loading sjd xll objects: " << e.what();
        Excel(xlcAlert, 0, 1, TempStrStl(err.str()));
        Excel(xlFree, 0, 1, &xDll);
        return 0;
    } catch (...) {
        Excel(xlFree, 0, 1, &xDll);
        return 0;
    }
}

DLLEXPORT int xlAutoClose() {

    static XLOPER xDll;

    try {
        Excel(xlGetName, &xDll, 0);

#ifdef XLL_STATIC
        // Unregister ObjectHandler functions
        unregisterOhFunctions(xDll);
#endif

        // Unregister QuantLib functions
        unregisterSJDFunctions(xDll);

        // Deallocate the Enumeration Registry
        QuantLibAddin::unregisterEnumerations();

#ifdef XLL_STATIC
        ObjectHandler::RepositoryXL::instance().clear();
#endif

        Excel(xlFree, 0, 1, &xDll);

        return 1;
    } catch (const std::exception &e) {
        std::ostringstream err;
        err << "Error unloading sjd xll objects: " << e.what();
        Excel(xlcAlert, 0, 1, TempStrStl(err.str()));
        Excel(xlFree, 0, 1, &xDll);
        return 0;
    } catch (...) {
        Excel(xlFree, 0, 1, &xDll);
        return 0;
    }
}
