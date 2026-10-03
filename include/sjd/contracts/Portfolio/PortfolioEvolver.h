#ifndef PORTFOLIOEVOLVER_INCLUDED
#define PORTFOLIOEVOLVER_INCLUDED

#include <list>
#include <algorithm>

#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/contracts/Portfolio/RegulatoryCapital.h>
#include <sjd/contracts/Portfolio/CVA/cva.h>
#include <sjd/rates/PathGenerators/SingleFactorGMB.h>
#include <sjd/rates/Discount/DiscountRateSource.h>

namespace sjd 
{
    // Every simulated value, for statistics computed outside the evolver. The arrays are
    // flat and row-major: spot, mtm and margin are [path][step], byTrade is
    // [path][step][trade]. margin is empty unless the portfolio is margined, and byTrade
    // is empty unless it was requested. A failed simulation has paths == 0.
    // With a holding period, dates also hold the margin reset dates. Exposure date k is
    // step exposureSteps[k], and its margin is read at step marginResetSteps[k].
    struct SimulatedPaths
    {
        std::vector<QuantLib::Date> dates;
        std::vector<size_t> exposureSteps;
        std::vector<size_t> marginResetSteps;
        size_t paths = 0;
        size_t steps = 0;
        size_t trades = 0;
        bool antithetic = false;
        std::vector<double> spot;
        std::vector<double> mtm;
        std::vector<double> margin;
        std::vector<double> byTrade;
    };

    class PortfolioEvolver 
    {
    public:
        // Use this for unmargined portfolios or portfolios that do not require Holding QuantLib::Period calculations
        PortfolioEvolver(boost::shared_ptr<Portfolio> portfolio, 
                         boost::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGenerator,
                         std::set<QuantLib::Date> forwardDates,
                         boost::shared_ptr<RegulatoryCapital> capital = boost::shared_ptr<RegulatoryCapital>());

        // Use this for portfolios that require Holding QuantLib::Period calculations
        PortfolioEvolver(boost::shared_ptr<Portfolio> portfolio, 
                         boost::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGenerator,
                         std::set<QuantLib::Date> forwardDates,
                         boost::shared_ptr<QuantLib::Calendar> calendarForHoldingPeriodCalcuation, 
                         size_t holdingPeriodInBusinessDays,
                         boost::shared_ptr<RegulatoryCapital> capital = boost::shared_ptr<RegulatoryCapital>());

        bool isOK();
        ValidationReport validationReport() const;
        ValidationReport validateReport();
        std::vector<std::string> getErrorMessages() const;
        std::string getErrorMessagesAsString() const;

        std::vector<QuantLib::Date> getSimulationDates();
        std::vector<QuantLib::Date> getExposureDates();

        void runMonteCarlo(unsigned long numberOfPaths);

        // With antithetic paths, path i and path i + paths/2 use opposite normals, and an
        // odd request is rounded up. The draws are in the same order as runMonteCarlo, so
        // the same seed gives the same paths.
        SimulatedPaths simulatePaths(unsigned long numberOfPaths, bool antithetic, bool byTrade);
        void getMonteCarloDetail(std::vector<QuantLib::Date>& simulationDates,
                                 std::vector<std::string>& columnHeadings, 
                                 std::vector<std::vector<double>>& data);
        void getMonteCarloAnalysis(std::vector<std::string> &columnHeadings, 
                                   std::vector<double> &data,
                                   boost::shared_ptr<DiscountRateSource> zeroCurve, 
                                   boost::shared_ptr<DiscountRateSource> spreadCurve,
                                   boost::shared_ptr<CVA> cva,
                                   boost::shared_ptr<CVA> dva);


        void getCIDetail(double confidenceInterval,
                         std::vector<QuantLib::Date>& simulationDates,
                         std::vector<std::string>& columnHeadings, 
                         std::vector<std::vector<double>>& data);

        void getCIAnalysis(double confidenceInterval,
                           std::vector<std::string> &columnHeadings, 
                           std::vector<double> &data,
                            boost::shared_ptr<DiscountRateSource> zeroCurve, 
                            boost::shared_ptr<DiscountRateSource> spreadCurve,
                           boost::shared_ptr<CVA> cva,
                           boost::shared_ptr<CVA> dva);

        void getSpotPathDetail(std::vector<double> spotPath,
                               std::vector<QuantLib::Date>& simulationDates,
                               std::vector<std::string>& columnHeadings, 
                               std::vector<std::vector<double>>& data);

        void getSpotPathAnalysis(std::vector<double> spotPath,
                                 std::vector<std::string> &columnHeadings, 
                                 std::vector<double> &data,
                                   boost::shared_ptr<DiscountRateSource> zeroCurve, 
                                  boost::shared_ptr<DiscountRateSource> spreadCurve,
                                 boost::shared_ptr<CVA> cva,
                                 boost::shared_ptr<CVA> dva);



    protected:
        void getPathDetail(boost::shared_ptr<Path> path,
                           std::vector<QuantLib::Date>& simulationDates,
                           std::vector<std::string>& columnHeadings, 
                           std::vector<std::vector<double>>& data);
        void getPathAnalysis(boost::shared_ptr<Path> path,
                             std::vector<std::string> &columnHeadings, 
                             std::vector<double> &data,
                               boost::shared_ptr<DiscountRateSource> zeroCurve, 
                              boost::shared_ptr<DiscountRateSource> spreadCurve,
                             boost::shared_ptr<CVA> cva,
                             boost::shared_ptr<CVA> dva);


        std::vector<double> getSpotAlongAPath(boost::shared_ptr<Path> path);
        std::vector<double> getMtMAlongAPath(boost::shared_ptr<Path> path);
        void getRegulatoryCapitalAlongAPath(boost::shared_ptr<Path> path, 
                                            std::vector<double>& regCap, 
                                            std::vector<double>& mtm, 
                                            std::vector<double>& margin);
        std::vector<double> margingBalanceForMtM(const std::vector<double> mtm);
        
        virtual void evolvePortfolioAlongPath(boost::shared_ptr<Path> path);
        void recordPath(SimulatedPaths& result,
                        size_t row,
                        boost::shared_ptr<Path> path,
                        const std::vector<boost::shared_ptr<SingleInstrumentContract>>& trades);
        virtual void collectStatsFromPath();
        virtual void accumulateStatistics(size_t i);
        virtual void calculateSummaryStats();

        void setSimulationDates(std::set<QuantLib::Date> dates); 
        virtual void initialiseAccumulators(size_t paths);

        double getXVAForExposurePath(std::vector<double> exposurePath, 
                                     boost::shared_ptr<CVA> cva, 
                                     boost::shared_ptr<DiscountRateSource> drs); 

        //dates MUST be sorted
        double getExpectedCostOfFundingPosition(std::vector<double> cashProfileToFund, 
                                                std::vector<QuantLib::Date> dates, 
                                                    boost::shared_ptr<DiscountRateSource> zeroCurve,
                                                boost::shared_ptr<DiscountRateSource> spreadCurve);

        boost::shared_ptr<sjd::IssueTracker> errorTracking;
        void recordIssue(const std::string& code,
                         const std::string& message,
                         const std::vector<std::string>& tail);

        bool portfolioIsMargined;
        bool portfolioAttractsCapital;

        boost::shared_ptr<Portfolio> portfolio;
        boost::shared_ptr<RegulatoryCapital> capitalModel;

        boost::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGenerator;
        boost::shared_ptr<QuantLib::Calendar> calendarForHoldingPeriod;
        size_t holdingPeriod;

        std::vector<QuantLib::Date> simulationDates;
        std::vector<size_t> holdingPeriodIndex; // those simulation dates needed only for holding period calculations
        std::vector<size_t> exposureDatesIndex; // dates on which exposures will be calculated
        size_t simulationSteps;
        size_t exposureSteps;


      // Tmp variables used when evaluating the portfolio data on a path. 
        std::vector<double> spotPath, 
                       mtmOnPath, 
                       marginOnPath, 
                       exposurePathNoOffset, 
                       negativeExposurePathNoOffset,
                       exposurePathWithOffset, 
                       negativeExposurePathWithOffset,
                       regCap;
      
        std::vector<std::vector<double> > pathAccumulator,
                                aveMtMAccumulator, 
                                marginAccumulator,
                                eeNoHoldingPeriodAccumulator, 
                                neeNoHoldingPeriodAccumulator,
                                eeWithHoldingPeriodAccumulator, 
                                neeWithHoldingPeriodAccumulator,
                                regCapAccumulator;      

      // Summary stats for MC output
        std::vector<double> averageSimulatedPath,
                       averageMtM, 
                       expectedMargin,
                       eeNoHoldingPeriod, 
                       neeNoHoldingPeriod,  
                       eeWithHoldingPeriod, 
                       neeWithHoldingPeriod,
                       eePFENoHoldingPeriod,
                       neePFENoHoldingPeriod, 
                       eePFEWithHoldingPeriod,
                       neePFEWithHoldingPeriod;

        //std::vector<double> riskWeightedAssets; 
    };

}

#endif
