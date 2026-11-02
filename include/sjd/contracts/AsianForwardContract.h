#pragma once

#include "EuropeanForwardContract.h"

namespace sjd
{

    /*======================================================================================
    AsianForward

     An Asian or average rate option with a generic averaging period
     =======================================================================================*/
    class AsianForward : public AsianContract, public ForwardContract
    {
    public:
        AsianForward(std::set<QuantLib::Date> averagingDates,
                     QuantLib::Date settlement,
                     double strike,
                     double volume,
                     BuySell buySell);

        AsianForward(QuantLib::Calendar calendar,
                     QuantLib::Date startOfAveraging,
                     QuantLib::Date endOfAveraging,
                     QuantLib::Date settlement,
                     double strike,
                     double volume,
                     BuySell buySell);

        AsianForward(AsianForward& forward);

        std::unique_ptr<Contract> clone() override;

        virtual ~AsianForward() {};

        virtual void validate();
        virtual void validate(std::shared_ptr<RatesEnvironment> re);
        virtual std::string validationRoot() const { return "asian_forward"; }

        virtual void setAveragingDates(std::set<QuantLib::Date> averagingDates);

        virtual double getValue(std::shared_ptr<RatesEnvironment> re);
        // Overwrite to ensure fwdFwdFactors are Multiplicative to the *average* of the Averaging Rates
        virtual void getFixingAndForwardRates(QuantLib::Date valuationDate,
                                              std::vector<double>& fixingRates,
                                              std::vector<double>& forwardRates,
                                              std::shared_ptr<RatesEnvironment> re);

        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re);

    protected:
        void constructorCommon(std::set<QuantLib::Date> averagingDates,
                               QuantLib::Date settlement,
                               double strike,
                               double volume,
                               BuySell buySell);
        void recordAsianIssue(const std::string& code,
                              const std::string& message,
                              const std::vector<std::string>& path) override;
        void absorbAsianReport(const ValidationReport& child,
                               const std::vector<std::string>& prefix) override;
    };

}
