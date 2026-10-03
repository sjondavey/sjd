#ifndef PORTFOLIOINSURANCE_INCLUDED
#define PORTFOLIOINSURANCE_INCLUDED
#pragma once

#include <vector>
#include <sjd/maths/maths.h>
#include <ql/time/date.hpp>
#include <sjd/rates/Discount/DiscountRateSource.h>
#include <memory>
#include <sjd/tools/Validation.h>

namespace sjd {

   /*======================================================================================
   Portfolio Insurance

    =======================================================================================*/
    class PortfolioInsurance : public Validatable
    {
    public:
        virtual ~PortfolioInsurance() {};
        
        void setParameters(QuantLib::Date policyStartDate, 
                           QuantLib::Date policyMaturityDate, 
                           std::shared_ptr<DiscountRateSource> standbyDRS, 
                           std::shared_ptr<DiscountRateSource> utilisationDRS);

        // The function that gets the sum insured should not return a negative value
        virtual double getSumInsured(QuantLib::Date date) = 0;
        std::shared_ptr<DiscountRateSource> getStandbyDRS();
        std::shared_ptr<DiscountRateSource> getUtilisationDRS();

      // Assumes all vectors are of the same length. 
        virtual void getInsurancePayments(std::vector<QuantLib::Date> dates, std::vector<double> mtm, std::vector<double> &insurancePayments);

        bool isOK();

    protected:
        std::string validationRoot() const override;
        void checkSizesAndCalcuateSpreads(std::vector<QuantLib::Date> dates, std::vector<double> mtm, std::vector<double> &insurancePayments);

        std::vector<QuantLib::Date> dates;
        std::vector<double> fwdFwdStandbyFactor, fwdFwdUtilisationFactor;


        QuantLib::Date policyStartDate;
        QuantLib::Date policyMaturityDate;
        std::shared_ptr<DiscountRateSource> standbyDRS;
        std::shared_ptr<DiscountRateSource> utilisationDRS;
        std::string className;
    };

    /*======================================================================================
   SimpleInsurance: Insurance paid in arrears based on the MtM at the end of the period

    =======================================================================================*/
    class SimpleInsurance : public PortfolioInsurance
    {
    public:
        SimpleInsurance(QuantLib::Date policyStartDate, QuantLib::Date policyMaturityDate, double constantSumInsured, 
                        std::shared_ptr<DiscountRateSource> standbyDRS, 
                        std::shared_ptr<DiscountRateSource> utilisationDRS);

        virtual double getSumInsured(QuantLib::Date date);

    private:

        double sumInsured;
    };

    /*======================================================================================
   SimpleInsuranceWithTermStructure: Insurance paid in arrears based on the MtM at the end 
    of the period

    =======================================================================================*/
    class SimpleInsuranceWithTermStructure : public PortfolioInsurance
    {
    public:
        SimpleInsuranceWithTermStructure(QuantLib::Date policyStartDate, QuantLib::Date policyMaturityDate, 
                                        std::vector<QuantLib::Date> dates, std::vector<double> insuredAmounts,
                                        std::shared_ptr<DiscountRateSource> standbyDRS, 
                                        std::shared_ptr<DiscountRateSource> utilisationDRS);

        virtual double getSumInsured(QuantLib::Date date);
        virtual bool isOK();

    private:

        std::vector<QuantLib::Date> dates;
        std::vector<double> insuredAmounts;
    };


}

#endif

