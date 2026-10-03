#ifndef SJD_OBJ_FIXINGRATESOURCE
#define SJD_OBJ_FIXINGRATESOURCE

#include <sjd/rates/Fixing/RollingFixingRateSource.h>
#include <oh/libraryobject.hpp>

using namespace std;

namespace sjdObjects 
{        
    class FixingRateSource : public ObjectHandler::LibraryObject<sjd::RollingFixingRateSource> 
    { 
    public:
        FixingRateSource(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                         boost::shared_ptr<sjd::RollingFixingRateSource> drs,
                         bool permanent);

        virtual vector<double> getRateSetOn(const vector<QuantLib::Date>& dates) const;

      bool isOK() const;
        string getErrorMessages() const;

        bool containsARateSetOn(QuantLib::Date date) const;

    protected: 
        OH_LIB_CTOR(FixingRateSource, sjd::RollingFixingRateSource) 
    };
}

#endif
