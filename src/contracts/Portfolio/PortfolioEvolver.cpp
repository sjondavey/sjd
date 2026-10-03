#include <sjd/contracts/Portfolio/PortfolioEvolver.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{
    PortfolioEvolver::PortfolioEvolver(boost::shared_ptr<Portfolio> portfolioInput, 
                                       boost::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGeneratorInput,
                                       set<Date> forwardDatesSet,
                                       boost::shared_ptr<RegulatoryCapital> capital)
    {
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("PortfolioEvolver"));
        portfolio = portfolioInput;
        portfolioIsMargined = portfolio->isMargined();
        capitalModel = capital;
        portfolioAttractsCapital = static_cast<bool>(capitalModel);
        pathGenerator = pathGeneratorInput;
        holdingPeriod = 0;
        setSimulationDates(forwardDatesSet);
    }

    PortfolioEvolver::PortfolioEvolver(boost::shared_ptr<Portfolio> portfolioInput, 
                                       boost::shared_ptr<SingleFactorEconomicScenarioGenerator> pathGeneratorInput,
                                       set<Date> forwardDatesSet,
                                       boost::shared_ptr<Calendar> calendarForHoldingPeriodInput, 
                                       size_t holdingPeriodInput,
                                       boost::shared_ptr<RegulatoryCapital> capital)
    {
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("PortfolioEvolver"));
        portfolio = portfolioInput;
        portfolioIsMargined = portfolio->isMargined();
        capitalModel = capital;
        portfolioAttractsCapital = static_cast<bool>(capitalModel);
        pathGenerator = pathGeneratorInput;
        holdingPeriod = holdingPeriodInput;
        calendarForHoldingPeriod = calendarForHoldingPeriodInput;
        setSimulationDates(forwardDatesSet);
    }

    std::string PortfolioEvolver::validationRoot() const
    {
        return "portfolio_evolver";
    }

    ValidationReport PortfolioEvolver::validateReport()
    {
        isOK();
        return validationReport();
    }

    bool PortfolioEvolver::isOK() 
    {
        errorTracking->clear();
        if ((holdingPeriod > 0) && !calendarForHoldingPeriod)
        {
            recordIssue("HOLDING_PERIOD_CALENDAR_MISSING",
                        "A holding period needs a calendar to count its business days",
                        {"holding_period"});
            return false;
        }
        if (simulationDates.size() == 0)
        {
            recordIssue("SIMULATION_DATES_EMPTY", "Simulation Dates are empty", {"simulation_dates"});
            return false;
        }
        if (simulationDates.front() != pathGenerator->getAnchorDate())
        {
            recordIssue("SIMULATION_ANCHOR_MISMATCH",
                        "First simulation date must be the anchor date",
                        {"simulation_dates"});
            return false;
        }
        portfolio->validate();
        if (!portfolio->isOK())
        {
            absorbChildReport(portfolio->validationReport(), {});
            return false;
        }
        set<Date> valuationDatesSet = set<Date>(simulationDates.begin(), simulationDates.end());
        pathGenerator->setSimulationDates(valuationDatesSet);
        if (!pathGenerator->isOK())
        {
            absorbChildReport(pathGenerator->validationReport(), {});
            return false;
        }
        boost::shared_ptr<Path> testPath = pathGenerator->getConfidenceInterval(0.95);
        for (size_t i = 0; i < testPath->getNumberOfSteps(); ++i)
        {
            portfolio->validate(testPath->getRatesEnvironment(i));
            if (!portfolio->isOK())
            {
                stringstream descriptionStream; 
                descriptionStream << "Portfolio cannot value against Rate Environment ";
                descriptionStream << i << ": " << portfolio->getErrorMessagesAsString();
                recordIssue("SIMULATION_MARKET_INVALID",
                            descriptionStream.str(),
                            {"simulation_steps", boost::lexical_cast<std::string>(i)});
                absorbChildReport(portfolio->validationReport(),
                                  {"simulation_steps", boost::lexical_cast<std::string>(i)});
                return false;
            }
        }
        return !errorTracking->getHasErrors();
    }

    vector<Date> PortfolioEvolver::getSimulationDates()
    {
        return simulationDates;
    }

    vector<Date> PortfolioEvolver::getExposureDates()
    {
        vector<Date> subset = vector<Date>(exposureSteps);
        for (size_t i = 0; i < exposureSteps; ++i)
        {
            subset[i] = simulationDates[exposureDatesIndex[i]];
        }
        return subset;
    }

    void PortfolioEvolver::runMonteCarlo(unsigned long N)
    {
        if (!validateReport().isOK())
        {
            return;
        }
        if (N == 0)
        {
            recordIssue("MONTE_CARLO_NO_PATHS", "Number of paths must be at least 1", {"number_of_paths"});
            return;
        }
        set<Date> valuationDatesSet = set<Date>(simulationDates.begin(), simulationDates.end());
        if (!pathGenerator->setSimulationDates(valuationDatesSet) || (!pathGenerator->isOK())) 
        {
            return;
        }
        // Paths come in antithetic pairs, so an odd request is rounded up.
        N += N % 2;
        initialiseAccumulators(N);    
        size_t halfway = N / 2;
        boost::shared_ptr<Path> path;
        for (size_t i = 0; i < halfway; ++i) 
        {
            path = pathGenerator->getRandomPath();
            evolvePortfolioAlongPath(path);
            collectStatsFromPath();
            accumulateStatistics(i);

            path = pathGenerator->getAntitheticPath();
            evolvePortfolioAlongPath(path);
            collectStatsFromPath();
            accumulateStatistics(halfway + i);
        }
        calculateSummaryStats();
    }

    SimulatedPaths PortfolioEvolver::simulatePaths(unsigned long N, bool antithetic, bool byTrade)
    {
        SimulatedPaths result;
        if (!validateReport().isOK())
        {
            return result;
        }
        if (N == 0)
        {
            recordIssue("MONTE_CARLO_NO_PATHS", "Number of paths must be at least 1", {"number_of_paths"});
            return result;
        }
        set<Date> valuationDatesSet = set<Date>(simulationDates.begin(), simulationDates.end());
        if (!pathGenerator->setSimulationDates(valuationDatesSet) || (!pathGenerator->isOK())) 
        {
            return result;
        }
        if (antithetic)
        {
            N += N % 2;
        }
        vector<boost::shared_ptr<SingleInstrumentContract>> trades;
        if (byTrade)
        {
            trades = portfolio->getUnderlyingSingleInstrumentContracts();
        }

        result.dates = simulationDates;
        result.exposureSteps = exposureDatesIndex;
        result.marginResetSteps = holdingPeriodIndex;
        result.paths = N;
        result.steps = simulationSteps;
        result.trades = trades.size();
        result.antithetic = antithetic;
        result.spot.assign(N * simulationSteps, 0.0);
        result.mtm.assign(N * simulationSteps, 0.0);
        if (portfolioIsMargined)
        {
            result.margin.assign(N * simulationSteps, 0.0);
        }
        result.byTrade.assign(N * simulationSteps * trades.size(), 0.0);

        size_t draws = antithetic ? N / 2 : N;
        for (size_t i = 0; i < draws; ++i) 
        {
            recordPath(result, i, pathGenerator->getRandomPath(), trades);
            if (antithetic)
            {
                recordPath(result, draws + i, pathGenerator->getAntitheticPath(), trades);
            }
        }
        return result;
    }

    void PortfolioEvolver::recordPath(SimulatedPaths& result,
                                      size_t row,
                                      boost::shared_ptr<Path> path,
                                      const vector<boost::shared_ptr<SingleInstrumentContract>>& trades)
    {
        evolvePortfolioAlongPath(path);
        size_t offset = row * result.steps;
        for (size_t j = 0; j < result.steps; ++j)
        {
            result.spot[offset + j] = spotPath[j];
            result.mtm[offset + j] = mtmOnPath[j];
            if (portfolioIsMargined)
            {
                result.margin[offset + j] = marginOnPath[j];
            }
            for (size_t k = 0; k < trades.size(); ++k)
            {
                result.byTrade[(offset + j) * trades.size() + k] =
                    trades[k]->getValue(path->getRatesEnvironment(j));
            }
        }
    }

    void PortfolioEvolver::getMonteCarloDetail(vector<Date>& datesToOutput,
                                               vector<string>& columnHeadings, 
                                               vector<vector<double>>& summaryStats)
    {
        datesToOutput = simulationDates;
        columnHeadings = vector<string>(0);
        summaryStats = vector<vector<double>>(0);
        if (averageSimulatedPath.size() == 0)
        {
            datesToOutput = vector<Date>(0);            
            return;
        }
        columnHeadings.push_back("Ave Path");
        columnHeadings.push_back("Ave MtM");
        if (portfolioIsMargined)
        {
            columnHeadings.push_back("Ave Margin");
        }
        columnHeadings.push_back("EE");
        columnHeadings.push_back("NEE");
        columnHeadings.push_back("EE with HP");
        columnHeadings.push_back("NEE with HP");
        columnHeadings.push_back("PFE");
        columnHeadings.push_back("-ve PFE");
        columnHeadings.push_back("PFE with HP");
        columnHeadings.push_back("-ve PFE with HP");
        if (portfolioAttractsCapital)
        {
            columnHeadings.push_back("Reg Cap");
        }         

        size_t exposureIndex = 0;
        vector<size_t>::iterator it;

        for (size_t i = 0; i < simulationDates.size(); ++i)
        {
            vector<double> resultsForDay;
            resultsForDay.push_back(averageSimulatedPath[i]);
            resultsForDay.push_back(averageMtM[i]);
            if (portfolioIsMargined)
            {
                resultsForDay.push_back(expectedMargin[i]);
            }
            it = find(exposureDatesIndex.begin(), exposureDatesIndex.end(), i);
            if (it != exposureDatesIndex.end())
            {
                resultsForDay.push_back(eeNoHoldingPeriod[exposureIndex]);
                resultsForDay.push_back(neeNoHoldingPeriod[exposureIndex]);
                resultsForDay.push_back(eeWithHoldingPeriod[exposureIndex]);
                resultsForDay.push_back(neeWithHoldingPeriod[exposureIndex]);
                resultsForDay.push_back(eePFENoHoldingPeriod[exposureIndex]);
                resultsForDay.push_back(neePFENoHoldingPeriod[exposureIndex]);
                resultsForDay.push_back(eePFEWithHoldingPeriod[exposureIndex]);
                resultsForDay.push_back(neePFEWithHoldingPeriod[exposureIndex]);
                if (portfolioAttractsCapital)
                {
                    resultsForDay.push_back(regCap[exposureIndex]);
                }
                ++exposureIndex;
            }
            summaryStats.push_back(resultsForDay);
        }
        return;
    }

    void PortfolioEvolver::getMonteCarloAnalysis(vector<string> &columnHeadings, 
                                                 vector<double> &summaryStats,
                                                   boost::shared_ptr<DiscountRateSource> zeroCurve, 
                                                 boost::shared_ptr<DiscountRateSource> spreadCurve,
                                                 boost::shared_ptr<CVA> cvaObj,
                                                 boost::shared_ptr<CVA> dvaObj)
    {
        columnHeadings = vector<string>(0);
        summaryStats = vector<double>(0);
        if (averageSimulatedPath.size() == 0)
        {
            columnHeadings = vector<string>(0);
            return;
        }

        if (portfolioIsMargined)
        {
            columnHeadings.push_back("CoF Margin");
            double m = getExpectedCostOfFundingPosition(expectedMargin, simulationDates, zeroCurve, spreadCurve);
            summaryStats.push_back(m);
        }
        if (cvaObj != 0)
        {
            columnHeadings.push_back("CVA");
            double cva = getXVAForExposurePath(eeNoHoldingPeriod, cvaObj, zeroCurve);
            summaryStats.push_back(cva);
            if (holdingPeriod > 0)
            {
                columnHeadings.push_back("CVA (with HP)");
                cva = getXVAForExposurePath(eeWithHoldingPeriod, cvaObj, zeroCurve);
                summaryStats.push_back(cva);
            }
            
        }
        if (dvaObj != 0)
        {
            columnHeadings.push_back("DVA");
            double dva = getXVAForExposurePath(neeNoHoldingPeriod, dvaObj, zeroCurve);
            summaryStats.push_back(dva);
            if (holdingPeriod > 0)
            {
                columnHeadings.push_back("DVA (with HP)");
                dva = getXVAForExposurePath(neeWithHoldingPeriod, dvaObj, zeroCurve);
                summaryStats.push_back(dva);
            }
        }
        if (portfolioAttractsCapital)
        {
            columnHeadings.push_back("CoF RegCap");
            double cofRegCap = getExpectedCostOfFundingPosition(regCap, getExposureDates(), zeroCurve, spreadCurve);
            summaryStats.push_back(cofRegCap);
        }
        return;
    }

    void PortfolioEvolver::getCIDetail(double confidenceInterval,
                                       vector<Date>& simulationDates,
                                       vector<string>& columnHeadings, 
                                       vector<vector<double>>& data)
    {
        boost::shared_ptr<Path> path = pathGenerator->getConfidenceInterval(confidenceInterval);
        getPathDetail(path, simulationDates, columnHeadings, data);
    }

    void PortfolioEvolver::getCIAnalysis(double confidenceInterval,
                                         vector<string> &columnHeadings, 
                                         std::vector<double> &data,
                                           boost::shared_ptr<DiscountRateSource> zc, 
                                          boost::shared_ptr<DiscountRateSource> spreadCurve,
                                         boost::shared_ptr<CVA> cva,
                                         boost::shared_ptr<CVA> dva)
    {
        boost::shared_ptr<Path> path = pathGenerator->getConfidenceInterval(confidenceInterval);
        getPathAnalysis(path, columnHeadings, data, zc, spreadCurve, cva, dva);
    }

    void PortfolioEvolver::getSpotPathDetail(vector<double> spotPath,
                                             vector<Date>& simulationDates,
                                             vector<string>& columnHeadings, 
                                             vector<vector<double>>& data)
    {
        boost::shared_ptr<Path> path = pathGenerator->getPathForSpot(spotPath);
        getPathDetail(path, simulationDates, columnHeadings, data);
    }

    void PortfolioEvolver::getSpotPathAnalysis(vector<double> spotPath,
                                               vector<string> &columnHeadings, 
                                               std::vector<double> &data,
                                                 boost::shared_ptr<DiscountRateSource> zc, 
                                                boost::shared_ptr<DiscountRateSource> spreadCurve,
                                               boost::shared_ptr<CVA> cva,
                                               boost::shared_ptr<CVA> dva)
    {
        boost::shared_ptr<Path> path = pathGenerator->getPathForSpot(spotPath);
        getPathAnalysis(path, columnHeadings, data, zc, spreadCurve, cva, dva);
    }

    void PortfolioEvolver::getPathDetail(boost::shared_ptr<Path> path,
                                         vector<Date>& datesToOutput,
                                         vector<string>& columnHeadings, 
                                         vector<vector<double>>& data)
    {
        vector<vector<double>> summaryStats = vector<vector<double>>(0);
        mtmOnPath = vector<double>(simulationSteps, 0);
        marginOnPath = vector<double>(simulationSteps, 0);
        exposurePathNoOffset = vector<double>(exposureSteps, 0);
        negativeExposurePathNoOffset = vector<double>(exposureSteps, 0);
        exposurePathWithOffset = vector<double>(exposureSteps, 0);
        negativeExposurePathWithOffset = vector<double>(exposureSteps, 0);
        regCap = vector<double>(exposureSteps, 0);

        evolvePortfolioAlongPath(path);
        collectStatsFromPath();
        
        datesToOutput = simulationDates;
        columnHeadings.clear();
        columnHeadings.push_back("Spot Path");
        columnHeadings.push_back("MtM");
        if (portfolioIsMargined)
        {
            columnHeadings.push_back("Margin");
        }
        columnHeadings.push_back("EE");
        columnHeadings.push_back("NEE");
        columnHeadings.push_back("EE with HP");
        columnHeadings.push_back("NEE with HP");
        if (portfolioAttractsCapital)
        {
            columnHeadings.push_back("Reg Cap");
        }         

        summaryStats = vector<vector<double>>(0);
        size_t exposureIndex = 0;
        vector<size_t>::iterator it;
        for (size_t i = 0; i < datesToOutput.size(); ++i)
        {
            vector<double> resultsForDay;
            resultsForDay.push_back(spotPath[i]);
            resultsForDay.push_back(mtmOnPath[i]);
            if (portfolioIsMargined)
            {
                resultsForDay.push_back(marginOnPath[i]);
            }
            it = find(exposureDatesIndex.begin(), exposureDatesIndex.end(), i);
            if (it != exposureDatesIndex.end())
            {
                resultsForDay.push_back(exposurePathNoOffset[exposureIndex]);
                resultsForDay.push_back(negativeExposurePathNoOffset[exposureIndex]);
                resultsForDay.push_back(exposurePathWithOffset[exposureIndex]);
                resultsForDay.push_back(negativeExposurePathWithOffset[exposureIndex]);
                if (portfolioAttractsCapital)
                {
                    resultsForDay.push_back(regCap[exposureIndex]);
                }
                ++exposureIndex;
            }
            summaryStats.push_back(resultsForDay);
        }
        data = summaryStats;
        return;
    }
   
    void PortfolioEvolver::getPathAnalysis(boost::shared_ptr<Path> path,
                                           vector<string> &columnHeadings, 
                                           std::vector<double> &summaryStats,
                                            boost::shared_ptr<DiscountRateSource> zeroCurve, 
                                            boost::shared_ptr<DiscountRateSource> spreadCurve,
                                           boost::shared_ptr<CVA> cvaObj,
                                           boost::shared_ptr<CVA> dvaObj)
    {
        summaryStats = vector<double>(0);
        spotPath = vector<double>(simulationSteps, 0);
        mtmOnPath = vector<double>(simulationSteps, 0);
        marginOnPath = vector<double>(simulationSteps, 0);
        exposurePathNoOffset = vector<double>(exposureSteps, 0);
        negativeExposurePathNoOffset = vector<double>(exposureSteps, 0);
        exposurePathWithOffset = vector<double>(exposureSteps, 0);
        negativeExposurePathWithOffset = vector<double>(exposureSteps, 0);
        regCap = vector<double>(exposureSteps, 0);

        evolvePortfolioAlongPath(path);
        collectStatsFromPath();

        columnHeadings = vector<string>(0);
        summaryStats = vector<double>(0);
        if (spotPath.size() == 0)
        {
            columnHeadings = vector<string>(0);
            return;
        }

        if (portfolioIsMargined)
        {
            columnHeadings.push_back("CoF Margin");
            double m = getExpectedCostOfFundingPosition(marginOnPath, simulationDates, zeroCurve, spreadCurve);
            summaryStats.push_back(m);
        }
        if (cvaObj != 0)
        {
            columnHeadings.push_back("CVA");
            double cva = getXVAForExposurePath(exposurePathNoOffset, cvaObj, zeroCurve);
            summaryStats.push_back(cva);
            if (holdingPeriod > 0)
            {
                columnHeadings.push_back("CVA (with HP)");
                cva = getXVAForExposurePath(exposurePathWithOffset, cvaObj, zeroCurve);
                summaryStats.push_back(cva);
            }
            
        }
        if (dvaObj != 0)
        {
            columnHeadings.push_back("DVA");
            double dva = getXVAForExposurePath(negativeExposurePathNoOffset, dvaObj, zeroCurve);
            summaryStats.push_back(dva);
            if (holdingPeriod > 0)
            {
                columnHeadings.push_back("DVA (with HP)");
                dva = getXVAForExposurePath(negativeExposurePathWithOffset, dvaObj, zeroCurve);
                summaryStats.push_back(dva);
            }
        }
        if (portfolioAttractsCapital)
        {
            columnHeadings.push_back("CoF RegCap");
            double cofRegCap = getExpectedCostOfFundingPosition(regCap, getExposureDates(), zeroCurve, spreadCurve);
            summaryStats.push_back(cofRegCap);
        }
        return;
    }

   void PortfolioEvolver::setSimulationDates(set<Date> dates)
   {
        holdingPeriodIndex.clear();
        exposureDatesIndex.clear();
        simulationSteps = 0;
        exposureSteps = 0;
        simulationDates = vector<Date>(dates.begin(), dates.end());       

        if ((holdingPeriod == 0) || !portfolioIsMargined || !calendarForHoldingPeriod)
        {
            for (size_t i = 0; i < simulationDates.size(); ++i)
            {
                exposureDatesIndex.push_back(i);
                holdingPeriodIndex.push_back(i);
            }
        }
        else
        {
            set<Date> exposureDatesSet = set<Date>(dates.begin(), dates.end());
            set<Date> merged = exposureDatesSet;
            vector<Date> regCapMarginResetDates;
            int numDays = holdingPeriod;
            set<Date>::iterator it;
            for (it = exposureDatesSet.begin(); it != exposureDatesSet.end(); ++it) 
            {
                Date marginResetDate = calendarForHoldingPeriod->advance(*it, -numDays, Days);
                if (marginResetDate < portfolio->getMargin()->getAnchorDate()) 
                {
                    marginResetDate = portfolio->getMargin()->getAnchorDate();
                }
                regCapMarginResetDates.push_back(marginResetDate);
                merged.insert(marginResetDate);
            }

            simulationDates = vector<Date>(merged.begin(), merged.end());

            vector<Date> exposureDates = vector<Date>(exposureDatesSet.begin(), exposureDatesSet.end());
            //vector<Date> regCapMarginResetDates = vector<Date>(regCapMarginResetDatesSet.begin(), regCapMarginResetDatesSet.end());

            exposureDatesIndex.clear();
            for (size_t i = 0; i < exposureDates.size(); ++i)
            {
                vector<Date>::iterator it;
                it = find(simulationDates.begin(), simulationDates.end(), exposureDates[i]);
                size_t index = std::distance(simulationDates.begin(), it);
                exposureDatesIndex.push_back(index);
            }
            holdingPeriodIndex.clear();
            for (size_t i = 0; i < regCapMarginResetDates.size(); ++i)
            {
                vector<Date>::iterator it;
                it = find(simulationDates.begin(), simulationDates.end(), regCapMarginResetDates[i]);
                size_t index = std::distance(simulationDates.begin(), it);
                holdingPeriodIndex.push_back(index);
            }
        }
        simulationSteps = simulationDates.size();
        exposureSteps = exposureDatesIndex.size();
   }

   vector<double> PortfolioEvolver::getSpotAlongAPath(boost::shared_ptr<Path> inputPath)
   {
       size_t steps = inputPath->getNumberOfSteps();
       vector<double> spot = vector<double>(steps);
       for (size_t i = 0; i < steps; ++i) 
       {
           Date ad = inputPath->getRatesEnvironment(i)->getAnchorDate();
           spot[i] = inputPath->getRatesEnvironment(i)->getFRS()->getForward(ad);
       }
       return spot;
   }

   vector<double> PortfolioEvolver::getMtMAlongAPath(boost::shared_ptr<Path> inputPath)
   {
       size_t steps = inputPath->getNumberOfSteps();
       vector<double> mtm = vector<double>(steps);
       for (size_t i = 0; i < steps; ++i) 
       {
           mtm[i] = portfolio->getValue(inputPath->getRatesEnvironment(i));
       }
       return mtm;
   }

   void PortfolioEvolver::getRegulatoryCapitalAlongAPath(boost::shared_ptr<Path> inputPath,
                                                         vector<double>& regCap, 
                                                         vector<double>& mtm, 
                                                         vector<double>& margin)
   {
       size_t steps = inputPath->getNumberOfSteps();
       regCap = vector<double>(exposureDatesIndex.size());
       mtm = vector<double>(simulationDates.size());
       margin = vector<double>(simulationDates.size());
       double initialMarginBalance = portfolio->getMarginBalance();

       size_t exposureCounter = 0;
       size_t holdingPeriodCounter = 0;
       for (size_t i = 0; i < steps; ++i) 
       {
           mtm[i] = portfolio->getValue(inputPath->getRatesEnvironment(i));
           if (portfolio->isMargined())
           {
               portfolio->getMargin()->updateBalanceForGivenMtM(mtm[i]);
               margin[i] = portfolio->getMarginBalance();
           }
           if ((exposureCounter < exposureDatesIndex.size()) && (i == exposureDatesIndex[exposureCounter]))
           {
               regCap[exposureCounter] = capitalModel->getRegulatoryCapital(inputPath->getRatesEnvironment(i));
               ++exposureCounter;
           }
           //if ((holdingPeriodCounter < holdingPeriodIndex.size()) && (i == holdingPeriodIndex[holdingPeriodCounter]))
           //{
           //    mtm[i] = portfolio->getValue(inputPath->getRatesEnvironment(i));
           //    if (portfolio->isMargined())
           //    {
           //        portfolio->getMargin()->updateBalanceForGivenMtM(mtm[i]);
           //        margin[i] = portfolio->getMarginBalance();
           //    }
           //    ++holdingPeriodCounter;
           //}
       }
       portfolio->setMarginBalance(initialMarginBalance);
       return;
   }

    vector<double> PortfolioEvolver::margingBalanceForMtM(const vector<double> futureMtMs)
    {
        vector<double> futureMarginBalance = vector<double>(futureMtMs.size(),0);
        portfolio->getMargin()->getFutureMarginBalancesAlongPath(futureMtMs, futureMarginBalance);        
        return futureMarginBalance;
    }

    void PortfolioEvolver::initialiseAccumulators(size_t paths)
    {      
        spotPath = vector<double>(simulationSteps, 0);
        mtmOnPath = vector<double>(simulationSteps, 0);
        marginOnPath = vector<double>(simulationSteps, 0);
        exposurePathNoOffset = vector<double>(exposureSteps, 0);
        negativeExposurePathNoOffset = vector<double>(exposureSteps, 0);
        exposurePathWithOffset  = vector<double>(exposureSteps, 0);
        negativeExposurePathWithOffset  = vector<double>(exposureSteps, 0);

        pathAccumulator.clear();
        aveMtMAccumulator.clear();
        marginAccumulator.clear();        
        eeNoHoldingPeriodAccumulator.clear();
        neeNoHoldingPeriodAccumulator.clear();
        eeWithHoldingPeriodAccumulator.clear();
        neeWithHoldingPeriodAccumulator.clear();
        regCapAccumulator.clear();

        vector<double> blankVector = vector<double>(paths, 0);
        for (size_t i = 0; i < simulationSteps; ++i) 
        {
            pathAccumulator.push_back(blankVector);
            aveMtMAccumulator.push_back(blankVector);
            marginAccumulator.push_back(blankVector);
        }
        for (size_t i = 0; i < exposureSteps; ++i) 
        {
            eeNoHoldingPeriodAccumulator.push_back(blankVector);
            neeNoHoldingPeriodAccumulator.push_back(blankVector);
            eeWithHoldingPeriodAccumulator.push_back(blankVector);
            neeWithHoldingPeriodAccumulator.push_back(blankVector);
            regCapAccumulator.push_back(blankVector);
        }

        averageSimulatedPath = vector<double>(simulationSteps,0);
        averageMtM = vector<double>(simulationSteps,0);
        expectedMargin = vector<double>(simulationSteps,0);
        eeNoHoldingPeriod = vector<double>(exposureSteps,0);
        neeNoHoldingPeriod = vector<double>(exposureSteps,0);
        eeWithHoldingPeriod = vector<double>(exposureSteps,0);
        neeWithHoldingPeriod = vector<double>(exposureSteps,0);
        eePFENoHoldingPeriod = vector<double>(exposureSteps,0);
        neePFENoHoldingPeriod = vector<double>(exposureSteps,0);
        eePFEWithHoldingPeriod = vector<double>(exposureSteps,0);
        neePFEWithHoldingPeriod = vector<double>(exposureSteps,0);
        regCap = vector<double>(exposureSteps, 0);
    }

   void PortfolioEvolver::evolvePortfolioAlongPath(boost::shared_ptr<Path> inputPath)
    {
        spotPath = getSpotAlongAPath(inputPath);
        if (portfolioAttractsCapital)
        {
            getRegulatoryCapitalAlongAPath(inputPath, regCap, mtmOnPath, marginOnPath);
        }
        else
        {
            mtmOnPath = getMtMAlongAPath(inputPath);
            if (portfolioIsMargined)
            {
                portfolio->getMargin()->getFutureMarginBalancesAlongPath(mtmOnPath, marginOnPath);
            }
        }
    }

   void PortfolioEvolver::collectStatsFromPath()
   {
       for (size_t j = 0; j < exposureSteps; ++j) 
       {
           double exposureWithOffset = mtmOnPath[exposureDatesIndex[j]];
           double exposureNoOffset = mtmOnPath[exposureDatesIndex[j]];            
           if (portfolioIsMargined)         
           {
              exposureWithOffset -= marginOnPath[holdingPeriodIndex[j]];
              exposureNoOffset -= marginOnPath[exposureDatesIndex[j]];
           }
           if (exposureWithOffset >= 0) 
           {
                exposurePathWithOffset[j] =  exposureWithOffset;
                negativeExposurePathWithOffset[j] = 0.0;
           }
           else 
           {
              exposurePathWithOffset[j] = 0.0;
              negativeExposurePathWithOffset[j] = exposureWithOffset;
           }
           if (exposureNoOffset >= 0) 
           {
              exposurePathNoOffset[j] =  exposureNoOffset;
              negativeExposurePathNoOffset[j] = 0.0;
           }
           else 
           {
              exposurePathNoOffset[j] = 0.0;
              negativeExposurePathNoOffset[j] = exposureNoOffset;
           }
        }
   }

    void PortfolioEvolver::accumulateStatistics(size_t i)
    {
        for (size_t j = 0; j < simulationSteps; ++j) 
        {
            pathAccumulator[j][i] = spotPath[j];
            aveMtMAccumulator[j][i] =  mtmOnPath[j];
            if (portfolioIsMargined) 
            {
                marginAccumulator[j][i] = marginOnPath[j];
            }
        }
        for (size_t j = 0; j < exposureSteps; ++j) 
        {
            if (portfolioAttractsCapital)
            {
                regCapAccumulator[j][i] = regCap[j];
            }
            eeNoHoldingPeriodAccumulator[j][i] =  exposurePathNoOffset[j];
            neeNoHoldingPeriodAccumulator[j][i] = negativeExposurePathNoOffset[j];
            eeWithHoldingPeriodAccumulator[j][i] =  exposurePathWithOffset[j];
            neeWithHoldingPeriodAccumulator[j][i] = negativeExposurePathWithOffset[j];
        }


    }
   
    void PortfolioEvolver::calculateSummaryStats()
    {
        double observations = (double) pathAccumulator[0].size();
        for (size_t i = 0; i < simulationSteps; ++i) 
        {
            averageSimulatedPath[i] = 
                accumulate(pathAccumulator[i].begin(), pathAccumulator[i].end(), 0.0) / observations;
            averageMtM[i] = 
                accumulate(aveMtMAccumulator[i].begin(), aveMtMAccumulator[i].end(), 0.0) / observations;
            if (portfolioIsMargined)
            {
                expectedMargin[i] = 
                        accumulate(marginAccumulator[i].begin(), marginAccumulator[i].end(), 0.0) / observations;
            }
        }

        for (size_t i = 0; i < exposureSteps; ++i) 
        {
            eeNoHoldingPeriod[i] = 
                accumulate(eeNoHoldingPeriodAccumulator[i].begin(), eeNoHoldingPeriodAccumulator[i].end(), 0.0) / observations;
            neeNoHoldingPeriod[i] = 
                accumulate(neeNoHoldingPeriodAccumulator[i].begin(), neeNoHoldingPeriodAccumulator[i].end(), 0.0) / observations;

            eeWithHoldingPeriod[i] = 
                accumulate(eeWithHoldingPeriodAccumulator[i].begin(), eeWithHoldingPeriodAccumulator[i].end(), 0.0) / observations;
            neeWithHoldingPeriod[i] = 
                accumulate(neeWithHoldingPeriodAccumulator[i].begin(), neeWithHoldingPeriodAccumulator[i].end(), 0.0) / observations;
         
         // PFE
            nth_element(eeNoHoldingPeriodAccumulator[i].begin(), 
                  eeNoHoldingPeriodAccumulator[i].begin()+int(eeNoHoldingPeriodAccumulator[i].size()*0.95), 
                  eeNoHoldingPeriodAccumulator[i].end());
            int x = int(eeNoHoldingPeriodAccumulator[i].size() * 0.95);
            eePFENoHoldingPeriod[i] = eeNoHoldingPeriodAccumulator[i][x];

            nth_element(eeWithHoldingPeriodAccumulator[i].begin(), 
                  eeWithHoldingPeriodAccumulator[i].begin()+int(eeWithHoldingPeriodAccumulator[i].size()*0.95), 
                  eeWithHoldingPeriodAccumulator[i].end());
            x = int(eeWithHoldingPeriodAccumulator[i].size() * 0.95);
            eePFEWithHoldingPeriod[i] = eeWithHoldingPeriodAccumulator[i][x];

            nth_element(neeNoHoldingPeriodAccumulator[i].begin(), 
                  neeNoHoldingPeriodAccumulator[i].begin()+int(neeNoHoldingPeriodAccumulator[i].size()*0.05), 
                  neeNoHoldingPeriodAccumulator[i].end());
            x = int(neeNoHoldingPeriodAccumulator[i].size() * 0.05);
            neePFENoHoldingPeriod[i] = neeNoHoldingPeriodAccumulator[i][x];

            nth_element(neeWithHoldingPeriodAccumulator[i].begin(), 
                  neeWithHoldingPeriodAccumulator[i].begin()+int(neeWithHoldingPeriodAccumulator[i].size()*0.05), 
                  neeWithHoldingPeriodAccumulator[i].end());
            x = int(neeWithHoldingPeriodAccumulator[i].size() * 0.05);
            neePFEWithHoldingPeriod[i] = neeWithHoldingPeriodAccumulator[i][x];

            if (portfolioAttractsCapital)
            {
                regCap[i] = accumulate(regCapAccumulator[i].begin(), regCapAccumulator[i].end(), 0.0) / observations;
                //regCap[i] *= regCapPortfolio->getRegCapCreditParameters()->getCapitalCharge() * 
                //                regCapPortfolio->getRegCapCreditParameters()->getCounterpartyWeighting();
            }
        }

   }
 

    double PortfolioEvolver::getXVAForExposurePath(vector<double> exposurePath, 
                                                   boost::shared_ptr<CVA> cvaObj, 
                                                   boost::shared_ptr<DiscountRateSource> drs) 
    {
        if (!cvaObj->isOK() || 
            !cvaObj->extendsTo(simulationDates.back()) || 
            !drs->extendsTo(simulationDates.back()))         
        {
            return numeric_limits<double>::quiet_NaN();
        }
        vector<Date> exposureDates;
        for (size_t i = 0; i < exposureSteps; ++i) 
        {
            exposureDates.push_back(simulationDates[exposureDatesIndex[i]]);
        }
        double cva = 0;
        for (size_t i = 1; i < exposureDates.size(); ++i) 
        {
            cva += drs->getDiscountFactor(exposureDates[i]) * 
                        cvaObj->getPD(exposureDates[i-1], exposureDates[i]) * 
                        exposurePath[i] *
                        cvaObj->getLGD(exposureDates[i]);
        }
        return cva;
    }

    double PortfolioEvolver::getExpectedCostOfFundingPosition(vector<double> position, 
                                                                vector<Date> dates, 
                                                              boost::shared_ptr<DiscountRateSource> zeroCurve,
                                                              boost::shared_ptr<DiscountRateSource> spreadCurve)
    {
        if ((position.size() != dates.size()) || 
            !zeroCurve->extendsTo(dates.back()) ||
            !spreadCurve->extendsTo(dates.back()))
        {
            return numeric_limits<double>::quiet_NaN();
        }
        double dft0 = zeroCurve->getDiscountFactor(dates[0]);
        double spreadDFt0 = spreadCurve->getDiscountFactor(dates[0]);
        double dft1 = dft0;
        double spreadDFt1 = spreadDFt0;

        double cost = (dft0 - spreadDFt0) * position[0];
        for (size_t i = 1; i < dates.size(); ++i) 
        {
            dft1 = zeroCurve->getDiscountFactor(dates[i]);
            spreadDFt1 = spreadCurve->getDiscountFactor(dates[i]);
            cost += dft0 * (dft1 / dft0 - spreadDFt1 / spreadDFt0) * position[i];
            dft0 = dft1;
            spreadDFt0 = spreadDFt1;
        }
        return cost;
    }

}