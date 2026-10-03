#ifndef SJD_OBJ_STRUCTURE
#define SJD_OBJ_STRUCTURE

#include <oh/libraryobject.hpp>
#include <sjd/contracts/Contracts.h>
#include <sjd/contracts/Cashflow.h>
#include <sjd/contracts/EuropeanForwardContract.h>
#include <sjd/contracts/AsianForwardContract.h>
#include <sjd/contracts/EuropeanOptionContract.h>
#include <sjd/contracts/AsianOptionContract.h>

#include <sjd/sjdObjects/RateSource/RatesEnvironment.h>


namespace sjdObjects 
{        
    /*======================================================================================
    Contract

    =======================================================================================*/
    class Contract : public ObjectHandler::LibraryObject<sjd::Contract> 
    { 
    public:
        bool isOK(boost::shared_ptr<sjdObjects::RatesEnvironment> re) const;
        string getErrorMessages() const; 

        double getValue(boost::shared_ptr<sjdObjects::RatesEnvironment> re);        

        string toString(boost::shared_ptr<sjdObjects::RatesEnvironment> re);

    protected: 
        OH_LIB_CTOR(Contract, sjd::Contract) 
    };


    
   /*======================================================================================
   FixedCashflowContract

    =======================================================================================*/
    class FixedCashflowContract : public Contract
    {
    public:      
        FixedCashflowContract(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                              Date cashflowDate, 
                              double volume,
                              sjd::BuySell buySell,
                              bool permanent);
                
    protected:
        OH_OBJ_CTOR(FixedCashflowContract, Contract);
    };

   /*======================================================================================
   ForwardContract

    =======================================================================================*/
    class ForwardContract : public Contract
    {
    public:      
        ForwardContract(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                        Date rateSetDate, 
                        Date settlement, 
                        double strike, 
                        double volume,
                        sjd::BuySell buySell,
                        bool permanent);
                
    protected:
        OH_OBJ_CTOR(ForwardContract, Contract);
    };

   /*======================================================================================
   AsianForwardContract

    =======================================================================================*/
    class AsianForwardContract : public Contract
    {
    public:      
        AsianForwardContract(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                            Calendar calendar,
                            Date startOfAveraging, 
                            Date endOfAveraging, 
                            Date settlement, 
                            double strike, 
                            double volume,
                            sjd::BuySell buySell,
                            bool permanent);
                
    protected:
        OH_OBJ_CTOR(AsianForwardContract, Contract);
    };

   /*======================================================================================
   OptionContract

    =======================================================================================*/
    class OptionContract : public Contract
    {
    public:      
        OptionContract(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                       Date optionMaturity, 
                       Date optionSettlement, 
                       double strike, 
                       sjd::PutCall type,
                       double volume,
                       Date premiumPaymentDate, 
                       double premiumAmount, 
                       sjd::BuySell buySell,
                       bool permanent);

    protected:
        OH_OBJ_CTOR(OptionContract, Contract);
    };

   /*======================================================================================
   AsianOptionContract

    =======================================================================================*/
    class AsianOptionContract : public Contract
    {
    public:
        AsianOptionContract(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,                      
                            Calendar calendar,
                       Date startOfAveraging, 
                            Date endOfAveraging, 
                            Date optionSettlement, 
                            double strike, 
                            sjd::PutCall type,
                            double volume,
                            Date premiumPaymentDate, 
                            double premiumAmount, 
                            sjd::BuySell buySell,
                            bool permanent);
                      
    protected:
        OH_OBJ_CTOR(AsianOptionContract, Contract);
    };

    /*======================================================================================
    MultipleInstrumentContract

    =======================================================================================*/
    class MultipleInstrumentContract : public Contract {
    public:
        
        MultipleInstrumentContract(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,                      
                                   vector<boost::shared_ptr<Contract>> inputs,
                                   bool permanent);

    protected:
        OH_OBJ_CTOR(MultipleInstrumentContract, Contract);
    };

}

#endif
