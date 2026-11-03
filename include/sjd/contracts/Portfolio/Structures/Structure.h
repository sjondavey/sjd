#pragma once

#include <list>

#include <sjd/contracts/Cashflow.h>
#include <sjd/contracts/Contracts.h>
#include <sjd/contracts/Portfolio/RegCap/RegCapCreditParameters.h>
#include <sjd/contracts/Portfolio/RegCap/RegCapUnderlyingFactors.h>

/*======================================================================================
Factors for add-ons: From Annex 4: Treatment of Counterparty Credit Risk

            Int Rates   FX&XAU  Equities    Precious    Other
                                            Metals      Commodities
                                            (ex xau)
    <= 1y       0.0%    1.0%    6.0%        7.0%        10.0%
    (1y,5y]     0.5%    5.0%    8.0%        7.0%        12.0%
    >=5y        1.5%    7.5%    10.0%       8.0%        15.0%

=======================================================================================*/

namespace sjd
{

    /*======================================================================================
    Structure
    Structures consist of one or more Single Instrument Contracts naturally grouped for
    regulatory capital purposes. This code only works for a final payoff profile that is
    piecewise linear i.e. it only works if the input contracts are forwards, regular or
    digital options.

    The two most important logical decisions this class must make regard the volume of the
    structure and the cap (if any). Determining the level of the cap, if it exists, is
    easy but determining the effective volume of a structure is not. Consider the example
    of a payoff structure _____/\_____ (buy one call at X1, sell 2 calls at X2 and buy
    one call at X3 with X1 < X2 < X3). What is the underlying volume of this structure
    1 or 2? This problem is potentailly compounded when this structure is in the
    "interior" of an uncapped structure. Consider the payoff \____/\____/ where the
    "volume" for the interior capped strucutre may be very large but the volume for the
    uncapped "wings" could be very low. Since the Addon is Spot x Volume x f(t) , getting
    the volume correct is important and failing to get it correct would incentivise
    economic arbitrage.

    The approach adoped here is to seperate the input contracts *assumed to have the
    same mautiry profile and rates* into at most two structures, one for the capped and
    one for the uncapped exposure. The volume of each structure will be determined
    by the gradient of the payoff profile over various ranges
    =======================================================================================*/
    class Structure : public MultipleInstrumentContract
    {
    public:
        Structure(std::shared_ptr<SingleInstrumentContract> underlyingContract,
                  std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors);

        Structure(std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts,
                  std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors);

        Structure(std::shared_ptr<Contract> underlyingContract,
                  std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors);

        Structure(Structure& structure);
        std::unique_ptr<Contract> clone() override;
        virtual ~Structure() {};

        void runChecks() override;
        void runChecks(RatesEnvironment& re) override;

        virtual QuantLib::Date getSettlementDate();
        virtual bool isExposureCapped() const;
        virtual bool getHasPriceExposure() const;
        virtual double getCap() const;
        // Effective volume used in the add-on, Spot x Volume x factor(t).
        virtual double getVolume() const;
        virtual double getPayoff(double forSpot);

        // To get the structure's reg cap, multiply the value returned from this function my
        // The couterparty weight and the capital charge
        virtual double getMtMPlusAddOn(std::shared_ptr<RatesEnvironment> re, double netGrossRatio = 1);
        // Often we already have the MtM so it's much more efficient just to use it than re-calcuate it
        virtual double getMtMPlusAddOnForGivenMtM(std::shared_ptr<RatesEnvironment> re,
                                                  double mtm,
                                                  double netGrossRatio = 1);

        // The following functions are static becuase they will be used both in the class
        // and also when manipulating a general collection of Single Instrument Contracts which may
        // need to be manipulated before it qualifies as a Structure
        static std::set<double> getAllStrikes(
            std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts);
        static std::vector<std::shared_ptr<SingleInstrumentContract>> getAllContractsWithStrike(
            std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts, double strike);
        // returns the signed gradient of the line passing though the three mtms corresponding to three input
        // spot values. If these do not fall on the same line then a NAN is returned
        static double gradient(std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts,
                               double x1,
                               double x2);
        // Piecewise linear is a fundamental assumption here so I need a specific test for it.
        static bool isPiecewiseLinear(
            std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts);
        // There is a issue to do with precision, especially when dealing with the gradients of the payoff
        // functions. The extent of the precision issues depends on the volume of the payoff function but
        // calculating this every time is painfull so this shortcut just finds the the maximum volume of each
        // of the contracts as a simple estimate of the volume of the collection.
        static double getMaximumNominalVolume(
            std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts);
        // Returns the largest absolute value gradient of the payoff of all the contracts at maturity, or the
        // Volume of the underlying collection of contracts
        static double getEffectiveVolume(
            std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts);
        static bool isMaximumVolumeInUncappedWing(
            std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts);

        static bool isUncappedOnTheLeft(
            std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts,
            double& effectiveVolumeAtLowestStrike);
        static bool isUncappedOnTheRight(
            std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts,
            double& effectiveVolumeAtHighestStrike);
        static bool isCapped(std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts,
                             double& cap,
                             bool& hasPriceExposure);

        // Groups all contracts by strike and finds the first strike where the contracts have have uncapped
        // exposure to the left. Returns all contracts with this strike
        static std::vector<std::shared_ptr<SingleInstrumentContract>>
        getContractsWithUncappedExposureOnTheLeft(
            std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts, double& commonStrike);
        static std::vector<std::shared_ptr<SingleInstrumentContract>>
        getContractsWithUncappedExposureOnTheRight(
            std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts, double& commonStrike);

        static bool isContractADerivative(std::shared_ptr<SingleInstrumentContract> underlyingContract);
        static bool isAnAsianDerivative(std::shared_ptr<SingleDerivativeContract> underlyingContract);
        // true if the underlying settlement / maturty profiles are the same and the rates environement is
        // compatable
        static bool qualifyForRegulatoryCapitalOffset(std::shared_ptr<SingleInstrumentContract> contract1,
                                                      std::shared_ptr<SingleInstrumentContract> contract2);
        static bool qualifyForRegulatoryCapitalOffset(std::shared_ptr<SingleInstrumentContract> contract1,
                                                      std::shared_ptr<SingleInstrumentContract> contract2,
                                                      std::string& reasonIfNot);
        static bool moveContracts(std::vector<std::shared_ptr<SingleInstrumentContract>> contractsToMove,
                                  std::vector<std::shared_ptr<SingleInstrumentContract>>& fromContracts,
                                  std::vector<std::shared_ptr<SingleInstrumentContract>>& toContracts);

        struct ExposureSplit
        {
            std::vector<std::shared_ptr<SingleInstrumentContract>> capped;
            std::vector<std::shared_ptr<SingleInstrumentContract>> uncapped;
        };

        // Takes contracts which are assumed to qualify for full or partial reg cap offset and builds
        // optimal structures by grouping according to volume.
        static Result<std::vector<std::shared_ptr<Structure>>> createStructures(
            std::vector<std::shared_ptr<SingleInstrumentContract>> contracts,
            std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors);

        // Copies allContracts and distributes them so that only the wings move into the uncapped
        // leg, and only in the minimum volume that leaves the remaining contracts capped.
        static Result<ExposureSplit> decomposeIntoCappedAndUncapped(
            const std::vector<std::shared_ptr<SingleInstrumentContract>>& allContracts);

        // Moves deferred premiums so the capped leg has the maximum mark-to-market and the uncapped
        // leg has the minimum.
        static Result<bool> optimiseCashflow(
            std::vector<std::shared_ptr<SingleInstrumentContract>>& cappedContracts,
            std::vector<std::shared_ptr<SingleInstrumentContract>>& unCappedContracts);

        virtual std::string toString(std::shared_ptr<RatesEnvironment> re);

    protected:
        void commonConstructor(std::vector<std::shared_ptr<SingleInstrumentContract>> underlyingContracts,
                               std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors);

        std::shared_ptr<RegCapUnderlyingFactors> regCapAssetFactors;
        std::vector<double> regCapFactorsFromForwardDates;
        bool cappedExposure, hasPriceExposure;
        double cap;
        double volume;
    };

}
