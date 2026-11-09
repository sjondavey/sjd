#pragma once

#include <ql/time/calendar.hpp>
#include <set>

#include <sjd/rates/RatesEnvironment.h>
#include <sjd/tools/Validation.h>

#include <sjd/tools/Common.h>

#include <memory>

namespace sjd
{

    /**
     * Something that can be validated against a RatesEnvironment and then valued.
     *
     * Do not inherit Contract directly. A single instrument inherits
     * SingleInstrumentContract. A group inherits MultipleInstrumentContract.
     *
     * validate() records issues. isOK() only reports whether any are stored.
     * getValue() assumes validate() has already succeeded. It does not store
     * the result. See docs/immutable-contracts.md.
     */
    class Contract : public Validatable
    {
    public:
        Contract() = default;
        virtual ~Contract() {};
        virtual std::unique_ptr<Contract> clone() const = 0;

        // validate() clears the tracker, runs the checks, and returns the report.
        // isOK() only reads that report.
        using Validatable::validate;
        using Validatable::validateReport;
        ValidationReport validate(RatesEnvironment& re);
        ValidationReport validate(const RatesEnvironment& re);
        ValidationReport validate(const std::shared_ptr<RatesEnvironment>& re);
        ValidationReport validateReport(std::shared_ptr<RatesEnvironment> re);
        std::string validationRoot() const override;

        void runChecks() override;
        virtual void runChecks(RatesEnvironment& re);
        virtual double getValue(const std::shared_ptr<RatesEnvironment>& re) const = 0;

        virtual std::string toString();
        virtual std::string toString(std::shared_ptr<RatesEnvironment> re);

        // These to be completed on the assumption that validate has been run and there
        // are no errors
        virtual std::string toStringContract() = 0;
        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re) = 0;
    };

    /**
     * One instrument. A portfolio or a structure is built out of these, not out of
     * further groups.
     *
     * getPayoff(spot) does not read a RatesEnvironment. Regulatory-capital grouping
     * uses that spot payoff, so it must not depend on which curves are loaded.
     */
    class SingleInstrumentContract : public Contract
    {
    public:
        virtual ~SingleInstrumentContract() {};
        std::unique_ptr<Contract> clone() const override = 0;

        void runChecks() override;
        void runChecks(RatesEnvironment& re) override;

        bool getHasPremium() const;
        QuantLib::Date getPremiumPaymentDate() const;
        double getPremiumAmount() const;

        QuantLib::Date getMaturityDate() const;
        QuantLib::Date getSettlementDate() const;

        BuySell getBuySell() const;
        double getBuySellAsDouble() const;

        double getVolume() const;
        // A new contract. This one is unchanged.
        std::shared_ptr<SingleInstrumentContract> withVolume(double volume) const;
        std::shared_ptr<SingleInstrumentContract> withPremiumAmount(double premium) const;

        // Used to decide if a particular structure's MtM is capped. This theoretical method
        // should be implemented to be independent of a rates environment
        virtual double getPayoff(double forSpot) const = 0;

        virtual std::string toStringContract();
        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re);

    protected:
        double volume = 0.0;
        double premiumAmount = 0.0;

        QuantLib::Date maturityDate;
        QuantLib::Date settlementDate;
        QuantLib::Date premiumPaymentDate;

        BuySell buySell = BuySell::BUY;

        bool hasPremium = false;

        void initialise(QuantLib::Date maturity,
                        QuantLib::Date settlement,
                        double volume,
                        BuySell buySell,
                        QuantLib::Date premiumPaymentDate,
                        double premiumAmount);
    };

    /**
     * A single instrument whose value moves with a forward curve. It has a strike.
     * validate(environment) fails when that environment has no forward source,
     * or when the source does not reach the settlement date.
     */
    class SingleDerivativeContract : public SingleInstrumentContract
    {
    public:
        std::unique_ptr<Contract> clone() const override = 0;
        virtual ~SingleDerivativeContract() {};

        void runChecks() override;
        void runChecks(RatesEnvironment& re) override;

        double getStrike() const;

        virtual std::string toStringContract();
        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re);

    protected:
        double strike = 0.0;

        void initialiseStrike(double strike);
    };

    /**
     * A derivative that also needs a volatility source.
     *
     * getOptionValue() is the option alone. getValue() adds the premium cashflow.
     * Put and call are a flag on this contract; the pricing formula itself is a
     * separate put type or call type.
     */
    class SingleOptionContract : public SingleDerivativeContract
    {
    public:
        virtual ~SingleOptionContract() {};

        void runChecks() override;
        void runChecks(RatesEnvironment& re) override;

        virtual double getPayoff(double forSpot) const;
        virtual double getOptionValue(const std::shared_ptr<RatesEnvironment>& re) const = 0;

        virtual double getValue(const std::shared_ptr<RatesEnvironment>& re) const;

        PutCall getType() const;

        virtual std::string toStringContract();
        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re);

    protected:
        PutCall type = PutCall::CALL;

        void initialiseType(PutCall type);
    };

    /**
     * Averaging dates for an Asian forward or Asian option. Mixed in beside the
     * instrument ladder. Averaging checks are written onto that instrument's
     * issue list, so an Asian forward or an Asian option has one tracker.
     *
     * On a valuation date, averaging dates on or before that date are fixings.
     * Averaging dates after it are forwards.
     */
    class AsianContract
    {
    public:
        virtual ~AsianContract() {};

        virtual void checkAveraging();
        virtual void checkAveraging(RatesEnvironment& re);

        std::vector<QuantLib::Date> getAveragingDates() const;

        std::set<QuantLib::Date> getAllBusinessDaysBetween(QuantLib::Date startDate,
                                                           QuantLib::Date endDate,
                                                           QuantLib::Calendar calendar);
        // Takes all averaging dates before or on the valuationDate and writes them to the
        // fixingDates and all averaging dates after the valuationDate and writes them to the
        // forwardDates
        virtual void getFixingAndForwardDates(QuantLib::Date valuationDate,
                                              std::vector<QuantLib::Date>& fixingDates,
                                              std::vector<QuantLib::Date>& forwardDates) const;

        virtual void getFixingAndForwardRates(QuantLib::Date valuationDate,
                                              std::vector<double>& fixingRates,
                                              std::vector<double>& forwardRates,
                                              std::shared_ptr<RatesEnvironment> re) const;

    protected:
        // The instrument writes these onto its own tracker, prefixed once.
        virtual void recordAsianIssue(std::string_view code,
                                      std::string_view message,
                                      const std::vector<std::string>& path) = 0;
        virtual void absorbAsianReport(const ValidationReport& child,
                                       const std::vector<std::string>& prefix) = 0;

        std::vector<QuantLib::Date> orderedAveragingDates;

        void storeAveragingDates(std::set<QuantLib::Date> averagingDates);
    };

    /**
     * A contract whose value is the sum of the single instruments it holds.
     * An empty group is not valid.
     *
     * clone() copies the group but keeps the same instrument pointers. A caller
     * that needs an independent book must clone the instruments first.
     * getUnderlyingSingleInstrumentContracts() returns those same live objects.
     */
    class MultipleInstrumentContract : public Contract
    {
    public:
        MultipleInstrumentContract() {};
        MultipleInstrumentContract(std::vector<std::shared_ptr<SingleInstrumentContract>> input);
        MultipleInstrumentContract(std::vector<std::shared_ptr<Contract>> input);
        MultipleInstrumentContract(const MultipleInstrumentContract& multipleInstrumentContract);
        // Keeps a reference the the same underlying contracts
        std::unique_ptr<Contract> clone() const override;
        virtual ~MultipleInstrumentContract() {};

        void runChecks() override;
        void runChecks(RatesEnvironment& re) override;

        virtual double getValue(const std::shared_ptr<RatesEnvironment>& re) const;

        virtual std::string toString(std::shared_ptr<RatesEnvironment> re);
        virtual std::string toStringContract();
        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re);

        // The instruments this group holds. Callers that change volume or premium
        // must clone first. Building a regulatory-capital structure does that.
        virtual std::vector<std::shared_ptr<SingleInstrumentContract>>
        getUnderlyingSingleInstrumentContracts() const;

    protected:
        virtual void setInputVariables();
        virtual std::vector<std::shared_ptr<SingleInstrumentContract>> getSingleInstrumentContracts(
            std::vector<std::shared_ptr<Contract>> input);

        std::vector<std::shared_ptr<SingleInstrumentContract>> contracts;
    };

    inline std::shared_ptr<SingleInstrumentContract> cloneInstrument(
        const std::shared_ptr<SingleInstrumentContract>& contract)
    {
        std::unique_ptr<Contract> cloned = contract->clone();
        return std::shared_ptr<SingleInstrumentContract>(
            static_cast<SingleInstrumentContract*>(cloned.release()));
    }

}
