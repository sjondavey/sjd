#ifndef ASIANOPTIONCONTRACT_INCLUDED
#define ASIANOPTIONCONTRACT_INCLUDED
#pragma once

#include <boost/math/special_functions/round.hpp>
#include <sjd/pricing/AsianOptionFormula.h>
#include <sjd/rates/Fixing/FixingRateSource.h>
#include <ql/time/calendar.hpp>

#include "EuropeanOptionContract.h"

namespace sjd {

   /*======================================================================================
   AsianOption
    
    An Asian or average rate option with a generic averaging period. Coded using ATM vols
    =======================================================================================*/
    class AsianOption : public SingleOptionContract, public AsianContract
    {
    public:
        AsianOption(std::set<QuantLib::Date> averagingDates, 
                           QuantLib::Date optionSettlement, 
                           double strike, 
                           PutCall type, 
                           double volume,
                           QuantLib::Date premiumPaymentDate, 
                           double premiumAmount, 
                           BuySell buySell);

        AsianOption(QuantLib::Calendar calendar, 
                    QuantLib::Date startOfAveraging, 
                    QuantLib::Date endOfAveraging, 
                    QuantLib::Date optionSettlement, 
                    double strike, 
                    PutCall type, 
                    double volume,
                    QuantLib::Date premiumPaymentDate, 
                    double premiumAmount, 
                    BuySell buySell);

        AsianOption(AsianOption &option);

        std::unique_ptr<Contract> clone() override;

        virtual ~AsianOption() {};

        virtual void validate();
        virtual std::string validationRoot() const { return "asian_option"; } 
        virtual void validate(std::shared_ptr<RatesEnvironment> re);

        virtual void setAveragingDates(std::set<QuantLib::Date> averagingDates); 
        virtual void setType(PutCall pc);

        //virtual double getOptionValue(QuantLib::Date valueDate, std::vector<double> sds);
        virtual double getOptionValue(std::shared_ptr<RatesEnvironment> re);

        virtual void getRates(QuantLib::Date valuationDate, 
                              std::vector<double> &fixingRates, 
                              std::vector<double> &forwardRates,
                              std::vector<double> &sd, 
                              std::vector<double> &vol,
                              std::shared_ptr<RatesEnvironment> re);

        virtual std::string toStringRates(std::shared_ptr<RatesEnvironment> re);

    protected:
        void constructorCommon(std::set<QuantLib::Date> averagingDates, 
                               QuantLib::Date optionSettlement, 
                               double strike, 
                               PutCall type, 
                               double volume,
                               QuantLib::Date premiumPaymentDate, 
                               double premiumAmount, 
                               BuySell buySell);
        void recordAsianIssue(const std::string& code,
                              const std::string& message,
                              const std::vector<std::string>& path) override;
        void absorbAsianReport(const ValidationReport& child,
                               const std::vector<std::string>& prefix) override;

       std::shared_ptr<AverageRateOption> option;
    };
}

#endif
