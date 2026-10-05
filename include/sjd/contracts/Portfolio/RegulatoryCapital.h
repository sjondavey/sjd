#ifndef SJD_REGULATORY_CAPITAL_INCLUDED
#define SJD_REGULATORY_CAPITAL_INCLUDED

#include <string>

#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/contracts/Portfolio/Structures/Structure.h>
#include <sjd/contracts/Portfolio/RegCap/RegCapUnderlyingFactors.h>
#include <sjd/contracts/Portfolio/RegCap/RegCapCreditParameters.h>

namespace sjd
{

    // Static description of one netting set. Its mark-to-market along a path is
    // the value of the structure's own contracts, not a weight on the book.
    struct NettingSet
    {
        QuantLib::Date settlement;
        double volume;
        bool hasPriceExposure;
        bool capped;
        double cap;
    };

    /*======================================================================================
    RegulatoryCapital

    Capital is calculated for a portfolio. It is not a kind of portfolio.
    =======================================================================================*/
    class RegulatoryCapital : public Validatable
    {
    public:
        virtual ~RegulatoryCapital() {}

        virtual bool isOK();
        virtual void validate();
        virtual void validate(std::shared_ptr<RatesEnvironment> re);

        virtual double getRegulatoryCapital(std::shared_ptr<RatesEnvironment> re) = 0;

    protected:
        std::string validationRoot() const override;
    };

    /*======================================================================================
    CurrentExposureMethod

    Basel II current exposure for one netting set. Holds the structures used for
    the add-on and reads margin from the same margin account as the portfolio.
    =======================================================================================*/
    class CurrentExposureMethod : public RegulatoryCapital
    {
    public:
        CurrentExposureMethod(std::vector<std::shared_ptr<Contract>> inputs,
                              std::vector<std::shared_ptr<SingleInstrumentContract>> extractedContracts,
                              std::shared_ptr<Margin> margin,
                              std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
                              std::shared_ptr<RegCapCreditParameters> regCapCreditParameters,
                              bool optimise);

        CurrentExposureMethod(const CurrentExposureMethod& other);

        bool isOK() override;
        void validate() override;
        void validate(std::shared_ptr<RatesEnvironment> re) override;
        double getRegulatoryCapital(std::shared_ptr<RatesEnvironment> re) override;

        bool hasStructureConversionError() const;
        const std::string& structureConversionErrorMessage() const;
        const std::vector<std::shared_ptr<Structure>>& structures() const;

        // Empty, with error set, when the contracts could not be turned into sets.
        std::vector<NettingSet> nettingSets(std::string& error) const;

        static std::vector<std::shared_ptr<Structure>> optimiseStructuresFromContracts(
            std::vector<std::shared_ptr<SingleInstrumentContract>> contracts,
            std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
            bool& successful,
            std::string& errorMessage);

    private:
        double getNGR(std::shared_ptr<RatesEnvironment> re, double& mtm);

        std::vector<std::shared_ptr<Structure>> nettingStructures;
        std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors;
        std::shared_ptr<RegCapCreditParameters> regCapCreditParameters;
        std::shared_ptr<Margin> margin;
        bool hasMargin;
        bool structureConversionError;
        std::string structureErrorMessage;
    };

    /*======================================================================================
    PortfolioB2CEM

    A valued portfolio plus a current-exposure capital model. The capital model
    sits behind the portfolio: this class is still a Portfolio, and it owns the
    CurrentExposureMethod rather than deriving the capital calculation through
    further portfolio subclasses.
    =======================================================================================*/
    class PortfolioB2CEM : public Portfolio
    {
    public:
        PortfolioB2CEM(std::vector<std::shared_ptr<Contract>> input,
                       std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
                       std::shared_ptr<RegCapCreditParameters> regCapCreditParameters,
                       bool optimise = false);

        PortfolioB2CEM(std::vector<std::shared_ptr<Contract>> input,
                       std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
                       std::shared_ptr<RegCapCreditParameters> regCapCreditParameters,
                       bool optimise,
                       std::shared_ptr<Margin> margin);

        PortfolioB2CEM(PortfolioB2CEM& portfolio);
        std::unique_ptr<Contract> clone() override;

        void validate() override;
        void validate(std::shared_ptr<RatesEnvironment> re) override;

        std::shared_ptr<RegulatoryCapital> regulatoryCapital() const;
        double getRegulatoryCapital(std::shared_ptr<RatesEnvironment> re);
        std::vector<NettingSet> nettingSets(std::string& error);
        std::vector<std::shared_ptr<Contract>> regulatoryStructures() const override;

    private:
        void attachCapital(std::vector<std::shared_ptr<Contract>> inputs,
                           std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
                           std::shared_ptr<RegCapCreditParameters> regCapCreditParameters,
                           bool optimise);

        std::shared_ptr<CurrentExposureMethod> capital;
    };

}

#endif
