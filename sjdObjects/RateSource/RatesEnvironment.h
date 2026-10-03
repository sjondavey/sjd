#ifndef SJD_OBJ_RATESENVIRONMENT
#define SJD_OBJ_RATESENVIRONMENT

#include <sjd/rates/RatesEnvironment.h>
#include <sjd/sjdObjects/RateSource/Forward/ForwardRateSource.h>
#include <sjd/sjdObjects/RateSource/Discount/DiscountRateSource.h>
#include <sjd/sjdObjects/RateSource/Fixing/FixingRateSource.h>
#include <sjd/sjdObjects/RateSource/Volatility/VolatilityRateSource.h>

#include <oh/libraryobject.hpp>

namespace sjdObjects 
{
        
   /*======================================================================================
    DiscountRateSource

    =======================================================================================*/
    class RatesEnvironment : public ObjectHandler::LibraryObject<sjd::RatesEnvironment> 
    { 
    public:
        RatesEnvironment(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                         Date anchorDate,
                         boost::shared_ptr<sjdObjects::DiscountRateSource> drs,
                         boost::shared_ptr<sjdObjects::DiscountRateSource> foreigndrs,
                         boost::shared_ptr<sjdObjects::ForwardRateSource> frs,
                         boost::shared_ptr<sjdObjects::VolatilityRateSource> vrs,
                         boost::shared_ptr<sjdObjects::FixingRateSource> fixingrs,
                         bool permanent);

        RatesEnvironment(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                         boost::shared_ptr<sjd::RatesEnvironment> re,
                         bool permanent);

        bool isOK();
        string getErrorMessages() const; 

        Date getAnchorDate();
        bool hasFRS();
        boost::shared_ptr<sjd::ForwardRateSource> getFRS();
        bool hasDRS();
        boost::shared_ptr<sjd::DiscountRateSource> getDRS();
        bool hasForeignDRS();
        boost::shared_ptr<sjd::DiscountRateSource> getForeignDRS();
        bool hasVRS();
        boost::shared_ptr<sjd::VolatilityRateSource> getVRS();
        bool hasFixingRS();
        boost::shared_ptr<sjd::FixingRateSource> getFixingRS();

    protected: 
        OH_LIB_CTOR(RatesEnvironment, sjd::RatesEnvironment) 

    };



}

#endif
