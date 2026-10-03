#ifndef SJD_OBJ_SINGLEFACTORGMB
#define SJD_OBJ_SINGLEFACTORGMB

#include <oh/libraryobject.hpp>
#include <sjd/rates/PathGenerators/SingleFactorGMB.h>
#include <sjd/sjdObjects/RateSource/RatesEnvironment.h>

namespace sjdObjects 
{

    class SingleFactorEconomicScenarioGenerator : public ObjectHandler::LibraryObject<sjd::SingleFactorEconomicScenarioGenerator> 
    {
    public:
        SingleFactorEconomicScenarioGenerator(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                                              const boost::shared_ptr<RatesEnvironment> re,
                                              bool permanent);

         bool isOK();
        string getErrorMessage();

        bool setSimulationDates(set<Date> simulationDates);
        void setRandomSeed(long seed);
        vector<boost::shared_ptr<sjd::RatesEnvironment>> getPathForCI(double confidenceInterval);
        vector<boost::shared_ptr<sjd::RatesEnvironment>> getPathForSpot(vector<double> spotPath);

    protected: 
        OH_LIB_CTOR(SingleFactorEconomicScenarioGenerator, sjd::SingleFactorEconomicScenarioGenerator)
    };
}

#endif
