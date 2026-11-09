#pragma once

#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/contracts/Portfolio/Structures/CemNettingSet.h>
#include <sjd/contracts/Portfolio/SupervisoryParameters.h>

namespace sjd
{

    // Static description of one netting set. Its mark-to-market along a path is
    // the value of the set's own contracts, not a weight on the book.
    struct NettingSet
    {
        QuantLib::Date settlement;
        double volume;
        bool hasPriceExposure;
        bool capped;
        double cap;
    };

    // One netting set's contribution. addOn is what remains in the exposure after
    // the set's mark-to-market is removed and replaced by the portfolio replacement cost.
    struct CapitalComponent
    {
        QuantLib::Date settlement;
        double volume = 0;
        bool capped = false;
        double markToMarket = 0;
        double addOn = 0;
    };

    struct CapitalResult
    {
        double ead = 0;
        double rwa = 0;
        double capital = 0;
        double replacement = 0;
        double netGrossRatio = 0;
        std::vector<CapitalComponent> components;
    };

    // A capital calculation for a portfolio. The portfolio does not know about it.
    class CapitalModel : public Validatable
    {
    public:
        ~CapitalModel() override = default;

        using Validatable::validate;
        ValidationReport validate(RatesEnvironment& re);
        ValidationReport validate(const std::shared_ptr<RatesEnvironment>& re);

        virtual void runChecks(RatesEnvironment& re);

        virtual CapitalResult capital(const Portfolio& portfolio,
                                      const Margin* margin,
                                      const std::shared_ptr<RatesEnvironment>& environment) const = 0;

        // Contracts the evolver values when the caller asks for per-set mark-to-market.
        virtual std::vector<std::shared_ptr<Contract>> nettingSetContracts() const = 0;

    protected:
        std::string validationRoot() const override;
    };

    // Basel II current exposure. Netting sets are built once. capital() values them
    // and reads the margin balance from the pointer for that call.
    class CurrentExposureMethod : public CapitalModel
    {
    public:
        CurrentExposureMethod(const Portfolio& portfolio,
                              AssetClass assetClass,
                              CounterpartyCredit credit,
                              bool optimise);

        // Each input becomes one netting set when optimise is false, including an
        // input that is itself a group of contracts. Optimise flattens them first.
        CurrentExposureMethod(std::vector<std::shared_ptr<Contract>> inputs,
                              AssetClass assetClass,
                              CounterpartyCredit credit,
                              bool optimise);

        void runChecks() override;
        void runChecks(RatesEnvironment& re) override;

        CapitalResult capital(const Portfolio& portfolio,
                              const Margin* margin,
                              const std::shared_ptr<RatesEnvironment>& environment) const override;

        std::vector<std::shared_ptr<Contract>> nettingSetContracts() const override;
        Result<std::vector<NettingSet>> nettingSets() const;

        std::string validationRoot() const override;

    private:
        void build(std::vector<std::shared_ptr<Contract>> inputs, bool optimise);

        static Result<std::vector<std::shared_ptr<CemNettingSet>>> optimiseSets(
            std::vector<std::shared_ptr<SingleInstrumentContract>> contracts, AssetClass assetClass);

        std::vector<std::shared_ptr<CemNettingSet>> nettingSets_;
        AssetClass assetClass;
        CounterpartyCredit credit;
        bool structureConversionError = false;
        std::string conversionDetailText;
    };

}
