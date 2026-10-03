#include <sjd/contracts/Portfolio/Insurance/PortfolioInsurance.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd {

    void PortfolioInsurance::setParameters(Date policyStartDateInput, Date policyMaturityDateInput,
                                           boost::shared_ptr<DiscountRateSource> standbyDRSInput, 
                                           boost::shared_ptr<DiscountRateSource> utilisationDRSInput)
    {
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("PortfolioInsurance"));

      policyStartDate = policyStartDateInput;
        policyMaturityDate = policyMaturityDateInput;
        standbyDRS = standbyDRSInput;
        utilisationDRS = utilisationDRSInput;         
   }

    boost::shared_ptr<DiscountRateSource> PortfolioInsurance::getStandbyDRS()
    {
        return standbyDRS;
    }

    boost::shared_ptr<DiscountRateSource> PortfolioInsurance::getUtilisationDRS()
    {
        return utilisationDRS;
    }

    void PortfolioInsurance::getInsurancePayments(vector<Date> dates, vector<double> mtm, vector<double> &insurancePayments)
   {
      checkSizesAndCalcuateSpreads(dates, mtm, insurancePayments);
      if (errorTracking->getHasErrors())
      {
         return;
      }
         for (size_t i = 0; i < dates.size(); ++i) 
         {
             double maxSumInsured = getSumInsured(dates[i]);
             double utilisation = min(mtm[i], maxSumInsured);
             utilisation = max(utilisation, 0.0);
             double standby = maxSumInsured - utilisation;
             insurancePayments[i] = utilisation * fwdFwdUtilisationFactor[i] + standby * fwdFwdStandbyFactor[i];
         }
   }

    bool PortfolioInsurance::isOK()
   {
      errorTracking->clear();
        if (policyStartDate > policyMaturityDate) 
        {
         errorTracking->record("INSURANCE_MATURITY_BEFORE_START", "Policy maturity is before policy start date", {"portfolio_insurance", "policy_maturity_date"});
        }

        if ((getSumInsured(policyStartDate) < -1e-14) || (getSumInsured(policyMaturityDate) < -1e-14)) 
      {
         errorTracking->record("INSURANCE_SUM_NEGATIVE", "Sum insured is negative", {"portfolio_insurance", "sum_insured"});
      }

        if (!standbyDRS->isOK()) 
        {
         errorTracking->absorb(standbyDRS->validationReport(), {"portfolio_insurance", "standby_discount_rate_source"});
        }

        if (!standbyDRS->extendsTo(policyMaturityDate)) 
        {
         errorTracking->record("INSURANCE_STANDBY_RANGE", "StandbyDRS does not extend to the policy maturity date", {"portfolio_insurance", "standby_discount_rate_source"});
        }

        if (!utilisationDRS->isOK())
        {
         errorTracking->absorb(utilisationDRS->validationReport(), {"portfolio_insurance", "utilisation_discount_rate_source"});
        }

        if (!utilisationDRS->extendsTo(policyMaturityDate)) 
        {
         errorTracking->record("INSURANCE_UTILISATION_RANGE", "UtilisationDRS does not extend to the policy maturity date", {"portfolio_insurance", "utilisation_discount_rate_source"});
        }

        if (standbyDRS->getAnchorDate() != utilisationDRS->getAnchorDate()) 
        {
         errorTracking->record("INSURANCE_ANCHOR_MISMATCH", "UtilisationDRS and StandbyDRS do not have the same anchor date", {"portfolio_insurance"});
        }
        return !errorTracking->getHasErrors();
   }

   vector<string> PortfolioInsurance::getErrorMessages() const
   {
      return errorTracking->getErrorMessages();
   }

    void PortfolioInsurance::checkSizesAndCalcuateSpreads(vector<Date> dates, vector<double> mtm, vector<double> &insurancePayments)
    {
         if ((dates.size() != mtm.size()) || (dates.size() != insurancePayments.size())) 
         {
                 insurancePayments = vector<double>(mtm.size(), 0);
                 errorTracking->record("INSURANCE_INPUT_DIMENSION", "Input vectors do not have the same dimension", {"portfolio_insurance", "inputs"});
                 return;
         }
         fwdFwdStandbyFactor.clear();
         fwdFwdStandbyFactor.push_back(1/standbyDRS->getDiscountFactor(dates[0])-1);
         fwdFwdUtilisationFactor.clear();
         fwdFwdUtilisationFactor.push_back(1/utilisationDRS->getDiscountFactor(dates[0])-1);
         for (size_t i = 1; i < dates.size(); ++i) {
            fwdFwdStandbyFactor.push_back(1/standbyDRS->getDiscountFactor(dates[i-1], dates[i])-1);
            fwdFwdUtilisationFactor.push_back(1/utilisationDRS->getDiscountFactor(dates[i-1], dates[i])-1);
         }
    }


    /*======================================================================================
   SimpleInsurance: Insurance paid in arrears based on the MtM at the end of the period

    =======================================================================================*/
    SimpleInsurance::SimpleInsurance(Date policyStartDate, Date policyMaturityDate, double sumInsuredInput, 
                                     boost::shared_ptr<DiscountRateSource> standbyDRS, 
                                     boost::shared_ptr<DiscountRateSource> utilisationDRS)
   {
      className = "SimpleInsurance";
        sumInsured = sumInsuredInput;
      setParameters(policyStartDate, policyMaturityDate, standbyDRS, utilisationDRS);
   }

    double SimpleInsurance::getSumInsured(Date date)
    {
        if ((date >= policyStartDate) && (date <= policyMaturityDate)) 
      {
            return sumInsured;
        }
        return 0.0;
    }

    /*======================================================================================
   SimpleInsuranceWithTermStructure: Insurance paid in arrears based on the MtM at the end 
    of the period

    =======================================================================================*/

    SimpleInsuranceWithTermStructure::SimpleInsuranceWithTermStructure(Date policyStartDate, Date policyMaturityDate, 
                                    vector<Date> datesInput, vector<double> insuredAmountsInput,
                                    boost::shared_ptr<DiscountRateSource> standbyDRS, 
                                    boost::shared_ptr<DiscountRateSource> utilisationDRS) 
    {
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("SimpleInsuranceWithTermStructure"));

        dates = datesInput;
        insuredAmounts = insuredAmountsInput;
      setParameters(policyStartDate, policyMaturityDate, standbyDRS, utilisationDRS);
    }

    bool SimpleInsuranceWithTermStructure::isOK()
    {
        PortfolioInsurance::isOK();
        if (dates.size() != insuredAmounts.size()) 
      {
         errorTracking->record("INSURANCE_DATES_AMOUNTS_DIMENSION", "Dates and insured amounts do not have the same dimension", {"portfolio_insurance", "insured_amounts"});
      }

        if (dates[0] < policyStartDate) 
      {
         errorTracking->record("INSURANCE_FIRST_DATE", "First insured date preceeds policy start date", {"portfolio_insurance", "dates"});
      }

        if (dates[dates.size()-1] != policyMaturityDate)
      {
            errorTracking->record("INSURANCE_LAST_DATE", "Last insured date must equal policy maturity date", {"portfolio_insurance", "dates"});
      }

        if (!isStrictlyIncreasing(dates)) 
      {
         errorTracking->record("INSURANCE_DATES_NOT_INCREASING", "Dates not strictly increasing", {"portfolio_insurance", "dates"});
      }

        for (size_t i = 0; i < insuredAmounts.size(); ++i) 
      {
            if (insuredAmounts[i] < -1e14) 
         {
                errorTracking->record("INSURANCE_AMOUNT_NEGATIVE", "At least one insured amount is negative", {"portfolio_insurance", "insured_amounts"});
         }
        }        
      return !errorTracking->getHasErrors();
    }

    double SimpleInsuranceWithTermStructure::getSumInsured(Date date)
    {
        if ((date >= policyStartDate) && (date <= policyMaturityDate)) 
      {
            if (date <= dates[0])
         {
                return insuredAmounts[0];
         }
          
            int ilo = 0;
          int ihi = dates.size() - 1;
          int i;
          while (ihi - ilo > 1) {
             i = (ihi + ilo) >> 1;
             if (dates[i] >= date) ihi = i;
             else ilo = i;
          }
            if (ihi == ilo) 
         {
                return numeric_limits<float>::quiet_NaN(); // shouldn't happen since we should check the x array is strictly monotonic but you never know
         }
            return insuredAmounts[ihi];

        }
        return 0.0;
    }

}