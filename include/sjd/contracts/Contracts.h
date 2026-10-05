#ifndef CONTRACTS_INCLUDED
#define CONTRACTS_INCLUDED

#pragma once

#include <ql/time/calendar.hpp>
#include <set>

#include <sjd/rates/RatesEnvironment.h>
#include <sjd/tools/Validation.h>

#include <sjd/tools/Common.h>

#include <sstream>
#include <iomanip>
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
     * getValue() assumes validate() has already succeeded.
     *
     * getValue() may reuse a number cached against the RatesEnvironment pointer
     * and that environment's market revision. Replacing a curve on the environment
     * advances the revision, and the next getValue reprices. Editing the numbers
     * inside a curve object that is already attached does not.
     */
    class Contract : public Validatable
    {
    public:
        Contract() {valueCalculated = false; cachedMarketRevision = 0;};
        virtual ~Contract() {};
        virtual std::unique_ptr<Contract> clone() = 0;

        // Call validate before checking for errors
        virtual bool isOK();
        ValidationReport validateReport();
        ValidationReport validateReport(std::shared_ptr<RatesEnvironment> re);
        std::string validationRoot() const override;

        virtual void validate();
        virtual void validate(std::shared_ptr<RatesEnvironment> re);
        virtual double getValue(std::shared_ptr<RatesEnvironment> re) = 0;        

        virtual std::string toString();
        virtual std::string toString(std::shared_ptr<RatesEnvironment> re);

        // These to be completed on the assumption that validate has been run and there 
        // are no errors
        virtual std::string toStringContract() = 0;
        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re) = 0;

    protected:
        // When we get into a MC simulaiton, especially with Regulatory capital, we will
        // often call the same getValue(re) multiple times; The following variables allow
        // a slow class to cache the contract value for a given rates environement
        bool valueCalculated;
        std::shared_ptr<RatesEnvironment> cachedRE;
        std::size_t cachedMarketRevision;
        double cachedContractValue;
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
        std::unique_ptr<Contract> clone() override = 0;

        virtual void validate();
        virtual void validate(std::shared_ptr<RatesEnvironment> re);
        
        bool getHasPremium() const;
        virtual void setPremiumPaymentDate(QuantLib::Date premiumPaymentDate);
        QuantLib::Date getPremiumPaymentDate() const;
        virtual void setPremiumAmount(double premium);
        double getPremiumAmount() const;

        virtual void setMaturityDate(QuantLib::Date maturityDate);
        QuantLib::Date getMaturityDate() const;

        virtual void setSettlementDate(QuantLib::Date settlementDate);
        QuantLib::Date getSettlementDate() const;

        void setBuySell(BuySell bs);
        BuySell getBuySell() const;                
        double getBuySellAsDouble() const;        

        double getVolume() const;
        virtual void setVolume(double volume);

        // Used to decide if a particular structure?s MtM is capped. This theoretical method 
        // should be implemented to be independent of a rates environment 
        virtual double getPayoff(double forSpot) = 0;

        virtual std::string toStringContract();
        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re);
    protected:

        double volume;
        double premiumAmount;
        double buySellAsDouble; // = +1 for buy / -1 for sell

        QuantLib::Date maturityDate;
        QuantLib::Date settlementDate;
        QuantLib::Date premiumPaymentDate;

        BuySell buySell;
        
        bool hasPremium; 
   };

    
    /**
     * A single instrument whose value moves with a forward curve. It has a strike.
     * validate(environment) fails when that environment has no forward source,
     * or when the source does not reach the settlement date.
     */
    class SingleDerivativeContract : public SingleInstrumentContract 
    {
    public:
        std::unique_ptr<Contract> clone() override = 0;
        virtual ~SingleDerivativeContract() {};

        virtual void validate();
        virtual void validate(std::shared_ptr<RatesEnvironment> re);

        virtual void setStrike(double strike);
        double getStrike() const;

        virtual std::string toStringContract();
        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re);
    protected:
        double strike;
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

        virtual void validate();
        virtual void validate(std::shared_ptr<RatesEnvironment> re);

        virtual double getPayoff(double forSpot);
        virtual double getOptionValue(std::shared_ptr<RatesEnvironment> re) = 0;
        
        virtual double getValue(std::shared_ptr<RatesEnvironment> re);
        
        virtual void setType(PutCall pc);
        PutCall getType() const;

        virtual std::string toStringContract();
        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re);
    protected:
        PutCall type;

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

        virtual void validate();
        virtual void validate(std::shared_ptr<RatesEnvironment> re);

        virtual void setAveragingDates(std::set<QuantLib::Date> averagingDates); 
        std::vector<QuantLib::Date> getAveragingDates() const; 

        std::set<QuantLib::Date> getAllBusinessDaysBetween(QuantLib::Date startDate, QuantLib::Date endDate, QuantLib::Calendar calendar);
        // Takes all averaging dates before or on the valuationDate and writes them to the 
        // fixingDates and all averaging dates after the valuationDate and writes them to the 
        // forwardDates
        virtual void getFixingAndForwardDates(QuantLib::Date valuationDate, 
                                              std::vector<QuantLib::Date> &fixingDates, 
                                              std::vector<QuantLib::Date> &forwardDates);

        virtual void getFixingAndForwardRates(QuantLib::Date valuationDate, 
                                              std::vector<double> &fixingRates, 
                                              std::vector<double> &forwardRates,
                                              std::shared_ptr<RatesEnvironment> re);


    protected:
        // The instrument writes these onto its own tracker, prefixed once.
        virtual void recordAsianIssue(const std::string& code,
                                      const std::string& message,
                                      const std::vector<std::string>& path) = 0;
        virtual void absorbAsianReport(const ValidationReport& child,
                                       const std::vector<std::string>& prefix) = 0;

        std::vector<QuantLib::Date> orderedAveragingDates;

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
        MultipleInstrumentContract(MultipleInstrumentContract &multipleInstrumentContract);
        // Keeps a reference the the same underlying contracts
        std::unique_ptr<Contract> clone() override;
        virtual ~MultipleInstrumentContract() {};
        
        virtual void validate();
        virtual void validate(std::shared_ptr<RatesEnvironment> re);

        virtual double getValue(std::shared_ptr<RatesEnvironment> re);
        

        virtual std::string toString(std::shared_ptr<RatesEnvironment> re);
        virtual std::string toStringContract();
        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re);

        // The instruments this group holds. Callers that change volume or premium
        // must clone first. Building a regulatory-capital structure does that.
        virtual std::vector<std::shared_ptr<SingleInstrumentContract>> getUnderlyingSingleInstrumentContracts();

    protected:
        virtual void setInputVariables();
        virtual std::vector<std::shared_ptr<SingleInstrumentContract>> getSingleInstrumentContracts(std::vector<std::shared_ptr<Contract>> input);


        std::        vector<std::shared_ptr<SingleInstrumentContract>> contracts;
    };

    inline std::shared_ptr<SingleInstrumentContract> cloneInstrument(
        const std::shared_ptr<SingleInstrumentContract>& contract)
    {
        std::unique_ptr<Contract> cloned = contract->clone();
        return std::shared_ptr<SingleInstrumentContract>(
            static_cast<SingleInstrumentContract*>(cloned.release()));
    }

}

#endif

