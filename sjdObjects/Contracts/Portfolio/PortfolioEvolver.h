#ifndef SJD_OBJ_PORTFOLIOEVOLVER
#define SJD_OBJ_PORTFOLIOEVOLVER

#include <sjd/contracts/Portfolio/PortfolioEvolver.h>

#include <sjd/sjdObjects/Contracts/Portfolio/Portfolio.h>
#include <sjd/sjdObjects/RateSource/PathGenerators/SingleFactorGMB.h>
#include <sjd/sjdObjects/Contracts/Portfolio/CVA/CVA.h>

using namespace std;
using namespace QuantLib;

namespace sjdObjects 
{

    /*======================================================================================
    PortfolioEvolver
    
    =======================================================================================*/
    class PortfolioEvolver : public ObjectHandler::LibraryObject<sjd::PortfolioEvolver> 
    {
    public:
        PortfolioEvolver(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                         boost::shared_ptr<Portfolio> portfolio, 
                         boost::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGenerator,
                         set<Date> forwardDates,
                         bool permanent);

        // Use this for portfolios that require Holding Period calculations
        PortfolioEvolver(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                         boost::shared_ptr<Portfolio> portfolio, 
                         boost::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGenerator,
                         set<Date> forwardDates,
                         size_t holdingPeriodInBusinessDays,
                         bool permanent);

        bool isOK();
        string getErrorMessages() const; 
        vector<Date> getSimulationDates();

        void runMonteCarlo(unsigned long numberOfPaths);
        bool hasMCBeenRun();
        void getMonteCarloDetail(vector<Date>& simulationDates,
                                 vector<string>& columnHeadings, 
                                 vector<vector<double>>& data);
        void getMonteCarloAnalysis(vector<string> &columnHeadings, 
                                   vector<double> &data,
                                   boost::shared_ptr<DiscountRateSource> zeroCurve, 
                                   boost::shared_ptr<DiscountRateSource> spreadCurve,
                                   boost::shared_ptr<CVA> cva,
                                   boost::shared_ptr<CVA> dva);

        void getCIDetail(double confidenceInterval,
                           vector<Date>& simulationDates,
                           vector<string>& columnHeadings, 
                           vector<vector<double>>& data);

        void getCIAnalysis(double confidenceInterval,
                             vector<string> &columnHeadings, 
                             std::vector<double> &data,
                               boost::shared_ptr<DiscountRateSource> zeroCurve, 
                              boost::shared_ptr<DiscountRateSource> spreadCurve,
                             boost::shared_ptr<CVA> cva,
                             boost::shared_ptr<CVA> dva);

        void getSpotPathDetail(vector<double> spotPath,
                               vector<Date>& simulationDates,
                               vector<string>& columnHeadings, 
                               vector<vector<double>>& data);

        void getSpotPathAnalysis(vector<double> spotPath,
                               vector<string> &columnHeadings, 
                               std::vector<double> &data,
                                boost::shared_ptr<DiscountRateSource> zeroCurve, 
                                boost::shared_ptr<DiscountRateSource> spreadCurve,
                               boost::shared_ptr<CVA> cva,
                               boost::shared_ptr<CVA> dva);

    protected:
        OH_LIB_CTOR(PortfolioEvolver, sjd::PortfolioEvolver);

        bool mcHasBeenRun;
    };
    
}

#endif
