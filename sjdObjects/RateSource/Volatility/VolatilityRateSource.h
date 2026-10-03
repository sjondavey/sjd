#ifndef SJD_OBJ_VOLATILITYRATESOURCE
#define SJD_OBJ_VOLATILITYRATESOURCE

#include <oh/libraryobject.hpp>
#include <sjd/rates/Volatility/VolatilityRateSource.h>
#include <sjd/rates/Volatility/DeterministicVolatilityRateSource.h>

namespace sjdObjects 
{        
   /*======================================================================================
    VolatilityRateSource

    =======================================================================================*/
    class VolatilityRateSource : public ObjectHandler::LibraryObject<sjd::VolatilityRateSource> 
    { 
    public:
        VolatilityRateSource(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                             boost::shared_ptr<sjd::VolatilityRateSource> vrs,
                             bool permanent);

        Date getAnchorDate();

        double getVolatility(QuantLib::Date toDate) const;
        double getVolatility(QuantLib::Date toDate, double strike, double forward) const;

        double getStandardDeviation(QuantLib::Date toDate) const;
        double getStandardDeviation(QuantLib::Date toDate, double strike, double forward) const;

        bool isOK() const;
        string getErrorMessages() const;

        bool isInRange(QuantLib::Date date) const;
        bool isInRange(Date date, double strike, double forward) const;

    protected: 
        OH_LIB_CTOR(VolatilityRateSource, sjd::VolatilityRateSource) 
    };

   /*======================================================================================
    DeterministicVolatilityRateSource

    =======================================================================================*/
    class DeterministicVolatilityRateSource : public VolatilityRateSource
    {
    public:
        virtual boost::shared_ptr<VolatilityRateSource> rollForward(
            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
            Date toDate,
            bool permanent);

        virtual boost::shared_ptr<VolatilityRateSource> parallelBump(
            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
            double spread,
            bool permanent);

    protected:
        OH_OBJ_CTOR(DeterministicVolatilityRateSource, VolatilityRateSource);
    };

}

#endif
