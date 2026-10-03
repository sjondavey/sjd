#ifndef SJD_OBJ_PORTFOLIO
#define SJD_OBJ_PORTFOLIO

#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/contracts/Portfolio/RegCap/RegCapCreditParameters.h>
#include <sjd/contracts/Portfolio/RegCap/RegCapUnderlyingFactors.h>

#include <sjd/sjdObjects/Contracts/Contracts.h>
#include <sjd/sjdObjects/RateSource/Discount/DiscountRateSource.h>
#include <sjd/sjdObjects/RateSource/Forward/ForwardRateSource.h>
#include <sjd/sjdObjects/RateSource/Volatility/VolatilityRateSource.h>

#include <sjd/sjdObjects/RateSource/PathGenerators/SingleFactorGMB.h>
#include <sjd/sjdObjects/Contracts/Portfolio/Margin/Margin.h>
#include <sjd/sjdObjects/Contracts/Portfolio/CVA/CVA.h>


using namespace std;
using namespace QuantLib;

namespace sjdObjects 
{

    /*======================================================================================
    Portfolio
    
    =======================================================================================*/
    class Portfolio : public Contract
    {
    public:
        Portfolio(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                  vector<boost::shared_ptr<Contract>> inputs,
                  bool permanent);

        Portfolio(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                  vector<boost::shared_ptr<Contract>> inputs,
                  boost::shared_ptr<Margin> margin,
                  bool permanent);

        bool isMargined();

        double getMarginBalance();

        double getMarginCall(boost::shared_ptr<sjdObjects::RatesEnvironment> re);

    protected:
        OH_OBJ_CTOR(Portfolio, Contract);
    };


    /*======================================================================================
    PortfolioWithRegulatoryCapital
    

    =======================================================================================*/
    class PortfolioWithRegulatoryCapital : public Portfolio
    {
    public:
        virtual double getRegulatoryCapital(boost::shared_ptr<sjdObjects::RatesEnvironment> re);
        /*
        virtual vector<double> getRegulatoryCapitalAlongPath(const vector<double> spotValues);

        virtual void runPortfolioSimulations(boost::shared_ptr<SingleFactorGBMPathGenerator> pathGen, 
                                     vector<Date> dates, 
                                     unsigned int numSims);

        virtual bool getPortfolioMCAnalysis(Date valueDate, 
                                            vector<string> &columnHeadings, 
                                            std::vector<double> &data,
                                 boost::shared_ptr<DiscountRateSource> zeroCurve, 
                                 boost::shared_ptr<DiscountRateSource> spreadCurve,
                                            boost::shared_ptr<CVA> cva = boost::shared_ptr<CVA>(),
                                            boost::shared_ptr<CVA> dva = boost::shared_ptr<CVA>());
        

      virtual bool getPortfolioPathAnalysis(Date valueDate, 
                                              vector<QuantLib::Date> dates, 
                                              vector<double> spotRates, 
                                              vector<string> &columnHeadings, 
                                              std::vector<double> &data,
                                   boost::shared_ptr<DiscountRateSource> zeroCurve, 
                                   boost::shared_ptr<DiscountRateSource> spreadCurve,
                                              boost::shared_ptr<CVA> cva = boost::shared_ptr<CVA>(),
                                              boost::shared_ptr<CVA> dva = boost::shared_ptr<CVA>());

        virtual bool getPortfolioPathStatistics(bool returnColumnHeadings, 
                                                bool returnDates, 
                                                vector<QuantLib::Date> dates, 
                                                vector<double> spotRates, 
                                                vector<string> &rowHeadings, 
                                                vector<string> &columnHeadings, 
                                                std::vector<std::vector<double> > &data);
    */
    protected:
        OH_OBJ_CTOR(PortfolioWithRegulatoryCapital, Portfolio);
    };
    

   /*======================================================================================
   Portfolio - Basel II, Current Exposure Method
    
    A portfolio consists of one or more structures
    =======================================================================================*/
    class PortfolioB2CEM : public PortfolioWithRegulatoryCapital
    {
    public:
        PortfolioB2CEM(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                        vector<boost::shared_ptr<Contract>> inputs,
                        boost::shared_ptr<sjd::RegCapUnderlyingFactors> regCapFactors,
                        boost::shared_ptr<sjd::RegCapCreditParameters> param,
                        bool optimise,
                        bool permanent);

        PortfolioB2CEM(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                        vector<boost::shared_ptr<Contract>> inputs,
                        boost::shared_ptr<sjd::RegCapUnderlyingFactors> regCapFactors,
                        boost::shared_ptr<sjd::RegCapCreditParameters> param,
                        bool optimise,
                        boost::shared_ptr<Margin> margin,
                        bool permanent);


    protected:
        OH_OBJ_CTOR(PortfolioB2CEM, PortfolioWithRegulatoryCapital);
    };

   /*======================================================================================
   Portfolio - Basel III, Current Exposure Method
    
    A portfolio consists of one or more structures
    =======================================================================================*
    class PortfolioB3CEM : public PortfolioWithRegulatoryCapital
    {
    public:
        PortfolioB3CEM(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                        vector<boost::shared_ptr<Contract>> inputs,
                        boost::shared_ptr<sjd::RegCapUnderlyingFactors> regCapFactors,
                        boost::shared_ptr<sjd::RegCapCreditParameters> param,
                        boost::shared_ptr<sjd::B3CreditRating> b3CreditRating,
                        bool optimise,
                        bool permanent);


    protected:
        OH_OBJ_CTOR(PortfolioB3CEM, PortfolioWithRegulatoryCapital);
    };
    */
}

#endif
