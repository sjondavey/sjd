#ifndef SJD_OBJ_CONTRACTSTRESSOR
#define SJD_OBJ_CONTRACTSTRESSOR

#include <sjd/contracts/ContractStressor.h>

#include <sjd/sjdObjects/Contracts/Contracts.h>
#include <sjd/sjdObjects/RateSource/RatesEnvironment.h>

using namespace std;
using namespace QuantLib;

namespace sjdObjects 
{

    /*======================================================================================
    ContractStressor
    
    =======================================================================================*/
    class ContractStressor : public ObjectHandler::LibraryObject<sjd::ContractStressor> 
    {
    public:
        ContractStressor(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                         boost::shared_ptr<Contract> contract, 
                         boost::shared_ptr<RatesEnvironment> rates,
                         bool permanent);


        bool isOK();
        string getErrorMessages() const; 
        vector<pair<string, double> > getStressedValues();

    protected:
        OH_LIB_CTOR(ContractStressor, sjd::ContractStressor);
    };
    
}

#endif
