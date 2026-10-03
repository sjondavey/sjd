#ifndef SJD_OBJ_FORWARDRATESOURCE
#define SJD_OBJ_FORWARDRATESOURCE

#include <sjd/rates/Forward/ForwardRateSource.h>
#include <sjd/rates/Forward/ForwardRateSourceDeterministic.h>
#include <sjd/rates/Forward/ForwardRateSourceStochastic.h>
#include <oh/libraryobject.hpp>

namespace sjdObjects 
{        

   /*======================================================================================
    ForwardRateSource

    =======================================================================================*/
    class ForwardRateSource : public ObjectHandler::LibraryObject<sjd::ForwardRateSource> 
    { 
    public:
        ForwardRateSource(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                          boost::shared_ptr<sjd::ForwardRateSource> frs,
                          bool permanent);

        std::vector<double> getForward(const std::vector<QuantLib::Date>& dates) const;

        bool isOK() const;
        string getErrorMessages() const;

        bool isInRange(QuantLib::Date date) const;

    protected: 
        OH_LIB_CTOR(ForwardRateSource, sjd::ForwardRateSource) 
    };

   /*======================================================================================
    ForwardRateSourceDeterministic

    =======================================================================================*/
    class ForwardRateSourceDeterministic : public ForwardRateSource
    {
    public:
        virtual boost::shared_ptr<ForwardRateSource> parallelBump(
                            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                            double spread,
                            bool permanent);


        virtual boost::shared_ptr<ForwardRateSource> rollForward(
                            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                            Date toDate,
                            bool permanent);

    protected:
        OH_OBJ_CTOR(ForwardRateSourceDeterministic, ForwardRateSource);
    };

   /*======================================================================================
    ForwardRateSourceStochastic

    =======================================================================================*/
    class ForwardRateSourceStochastic : public ForwardRateSource
    {
    public:

        virtual vector<boost::shared_ptr<ForwardRateSource> > rollForward(
                                                const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                                                set<Date> toDates,
                                                vector<double> normalRVs,
                                                bool permanent);

    protected:
        OH_OBJ_CTOR(ForwardRateSourceStochastic, ForwardRateSource);
    };
}

#endif
