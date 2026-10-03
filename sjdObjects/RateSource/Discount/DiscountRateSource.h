#ifndef SJD_OBJ_DISCOUNTINGRATESOURCE
#define SJD_OBJ_DISCOUNTINGRATESOURCE

#include <sjd/rates/Discount/DiscountRateSource.h>
#include <sjd/rates/Discount/DiscountCurves.h>

#include <oh/libraryobject.hpp>

namespace sjdObjects 
{
        
   /*======================================================================================
    DiscountRateSource

    =======================================================================================*/
    class DiscountRateSource : public ObjectHandler::LibraryObject<sjd::DiscountRateSource> 
    { 
    public:
        DiscountRateSource(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                           boost::shared_ptr<sjd::DiscountRateSource> drs,
                           bool permanent);

        std::vector<double> getDiscountFactor(const std::vector<QuantLib::Date>& dates) const;

        bool isOK() const;
        string getErrorMessages() const;

        bool isInRange(QuantLib::Date date) const;

    protected: 
        OH_LIB_CTOR(DiscountRateSource, sjd::DiscountRateSource) 


    };


   /*======================================================================================
    DeterministicDiscountRateSource

    =======================================================================================*/
    class DeterministicDiscountRateSource : public DiscountRateSource
    {
    public:
        virtual boost::shared_ptr<DiscountRateSource> parallelBump(
                            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                            double spread,
                            boost::shared_ptr<QuantLib::DayCounter> dayCounter, 
                            Compounding compounding, 
                            Frequency frequency,
                            bool permanent) = 0;


        virtual boost::shared_ptr<DiscountRateSource> rollForward(
                            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                            Date toDate,
                            bool permanent) = 0;

    protected:
        OH_OBJ_CTOR(DeterministicDiscountRateSource, DiscountRateSource);
    };

}

#endif
