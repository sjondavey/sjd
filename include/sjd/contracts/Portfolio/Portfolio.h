#ifndef PORTFOLIO_INCLUDED
#define PORTFOLIO_INCLUDED
// #pragma once

#include <sjd/contracts/Contracts.h>
#include <sjd/contracts/Portfolio/Margin/Margin.h>

namespace sjd 
{

    /*======================================================================================
    Portfolio
    
    A portfolio consists of one or more single instrument contracts. A Portfolio only 
    addresses issues of forward valuation of the instruments. Regulatory capital is
    calculated by a RegulatoryCapital model that sits behind the portfolio.
    =======================================================================================*/
    class Portfolio : public MultipleInstrumentContract
    {
    public:
        Portfolio() {};
        Portfolio(std::vector<boost::shared_ptr<SingleInstrumentContract>> input);
        Portfolio(std::vector<boost::shared_ptr<SingleInstrumentContract>> input, 
                  boost::shared_ptr<Margin> margin);
        Portfolio(std::vector<boost::shared_ptr<Contract>> input);
        Portfolio(std::vector<boost::shared_ptr<Contract>> input,
                  boost::shared_ptr<Margin> margin);
        Portfolio(Portfolio &Portfolio);
        // Keeps a reference the the same underlying contracts
        std::unique_ptr<Contract> clone() override;
        virtual ~Portfolio() {};

        virtual void validate();
        virtual void validate(boost::shared_ptr<RatesEnvironment> re);
        virtual std::string validationRoot() const { return "portfolio"; }

        bool isMargined();
        boost::shared_ptr<Margin> getMargin();
        void setMarginBalance(double balance);
        double getMarginBalance();
        double getMarginCall(boost::shared_ptr<RatesEnvironment> re);
        void updateMarginBalanceWithCall(double marginCall);
                                         
        std::vector<boost::shared_ptr<SingleInstrumentContract>> getUnderlyingSingleInstrumentContracts();

    protected:
        virtual void setInputVariables();
        
        bool hasMargin;
        boost::shared_ptr<Margin> margin;
   };

}

#endif
