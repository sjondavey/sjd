#pragma once

#include <sjd/contracts/Contracts.h>
#include <sjd/contracts/Portfolio/Margin/Margin.h>

namespace sjd
{

    /*======================================================================================
    Portfolio

    A portfolio consists of one or more single instrument contracts, and an optional
    margin account. It prices the book. Regulatory capital is a CapitalModel, which
    is not a portfolio.
    =======================================================================================*/
    class Portfolio : public MultipleInstrumentContract
    {
    public:
        Portfolio() {};
        Portfolio(std::vector<std::shared_ptr<SingleInstrumentContract>> input);
        Portfolio(std::vector<std::shared_ptr<SingleInstrumentContract>> input,
                  std::shared_ptr<Margin> margin);
        Portfolio(std::vector<std::shared_ptr<Contract>> input);
        Portfolio(std::vector<std::shared_ptr<Contract>> input, std::shared_ptr<Margin> margin);
        Portfolio(const Portfolio& portfolio);
        // Keeps a reference the the same underlying contracts
        std::unique_ptr<Contract> clone() const override;
        virtual ~Portfolio() {};

        void runChecks() override;
        void runChecks(RatesEnvironment& re) override;
        virtual std::string validationRoot() const { return "portfolio"; }

        bool isMargined() const;
        std::shared_ptr<Margin> getMargin();
        void setMarginBalance(double balance);
        double getMarginBalance() const;
        double getMarginCall(std::shared_ptr<RatesEnvironment> re);
        void updateMarginBalanceWithCall(double marginCall);

    protected:
        virtual void setInputVariables();

        bool hasMargin = false;
        std::shared_ptr<Margin> margin;
    };

}
