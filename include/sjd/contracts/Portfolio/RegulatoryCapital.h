#ifndef SJD_REGULATORY_CAPITAL_INCLUDED
#define SJD_REGULATORY_CAPITAL_INCLUDED

#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/contracts/Portfolio/Structures/Structure.h>
#include <sjd/contracts/Portfolio/RegCap/RegCapUnderlyingFactors.h>
#include <sjd/contracts/Portfolio/RegCap/RegCapCreditParameters.h>

namespace sjd
{

    /*======================================================================================
    RegulatoryCapital

    Capital is calculated for a portfolio. It is not a kind of portfolio.
    =======================================================================================*/
    class RegulatoryCapital
    {
    public:
        virtual ~RegulatoryCapital() {}

        virtual bool isOK();
        virtual void validate();
        virtual void validate(boost::shared_ptr<RatesEnvironment> re);
        virtual ValidationReport validationReport() const;

        virtual double getRegulatoryCapital(boost::shared_ptr<RatesEnvironment> re) = 0;

    protected:
        boost::shared_ptr<sjd::IssueTracker> errorTracking;
    };

    /*======================================================================================
    CurrentExposureMethod

    Basel II current exposure for one netting set. Holds the structures used for
    the add-on and reads margin from the same margin account as the portfolio.
    =======================================================================================*/
    class CurrentExposureMethod : public RegulatoryCapital
    {
    public:
        CurrentExposureMethod(std::vector<boost::shared_ptr<Contract>> inputs,
                              std::vector<boost::shared_ptr<SingleInstrumentContract>> extractedContracts,
                              boost::shared_ptr<Margin> margin,
                              boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
                              boost::shared_ptr<RegCapCreditParameters> regCapCreditParameters,
                              bool optimise);

        CurrentExposureMethod(const CurrentExposureMethod& other);

        bool isOK() override;
        void validate() override;
        void validate(boost::shared_ptr<RatesEnvironment> re) override;
        ValidationReport validationReport() const override;
        double getRegulatoryCapital(boost::shared_ptr<RatesEnvironment> re) override;

        bool hasStructureConversionError() const;
        const std::string& structureConversionErrorMessage() const;
        const std::vector<boost::shared_ptr<Structure>>& structures() const;

        static std::vector<boost::shared_ptr<Structure>> optimiseStructuresFromContracts(
            std::vector<boost::shared_ptr<SingleInstrumentContract>> contracts,
            boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
            bool& successful,
            std::string& errorMessage);

    private:
        double getNGR(boost::shared_ptr<RatesEnvironment> re, double& mtm);

        std::vector<boost::shared_ptr<Structure>> nettingStructures;
        boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors;
        boost::shared_ptr<RegCapCreditParameters> regCapCreditParameters;
        boost::shared_ptr<Margin> margin;
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
        PortfolioB2CEM(std::vector<boost::shared_ptr<Contract>> input,
                       boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
                       boost::shared_ptr<RegCapCreditParameters> regCapCreditParameters,
                       bool optimise = false);

        PortfolioB2CEM(std::vector<boost::shared_ptr<Contract>> input,
                       boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
                       boost::shared_ptr<RegCapCreditParameters> regCapCreditParameters,
                       bool optimise,
                       boost::shared_ptr<Margin> margin);

        PortfolioB2CEM(PortfolioB2CEM& portfolio);
        std::unique_ptr<Contract> clone() override;

        void validate() override;
        void validate(boost::shared_ptr<RatesEnvironment> re) override;

        boost::shared_ptr<RegulatoryCapital> regulatoryCapital() const;
        double getRegulatoryCapital(boost::shared_ptr<RatesEnvironment> re);

    private:
        void attachCapital(std::vector<boost::shared_ptr<Contract>> inputs,
                           boost::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors,
                           boost::shared_ptr<RegCapCreditParameters> regCapCreditParameters,
                           bool optimise);

        boost::shared_ptr<CurrentExposureMethod> capital;
    };

}

#endif
