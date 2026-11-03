#include <sjd/contracts/Portfolio/Insurance/PortfolioInsurance.h>

#include <sjd/tools/Errors.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    void PortfolioInsurance::setParameters(Date policyStartDateInput,
                                           Date policyMaturityDateInput,
                                           std::shared_ptr<DiscountRateSource> standbyDRSInput,
                                           std::shared_ptr<DiscountRateSource> utilisationDRSInput)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("PortfolioInsurance");

        policyStartDate = policyStartDateInput;
        policyMaturityDate = policyMaturityDateInput;
        standbyDRS = standbyDRSInput;
        utilisationDRS = utilisationDRSInput;
    }

    std::shared_ptr<DiscountRateSource> PortfolioInsurance::getStandbyDRS()
    {
        return standbyDRS;
    }

    std::shared_ptr<DiscountRateSource> PortfolioInsurance::getUtilisationDRS()
    {
        return utilisationDRS;
    }

    void PortfolioInsurance::getInsurancePayments(vector<Date> paymentDates,
                                                  vector<double> mtm,
                                                  vector<double>& insurancePayments)
    {
        checkSizesAndCalcuateSpreads(paymentDates, mtm, insurancePayments);
        if (errorTracking->getHasErrors())
        {
            return;
        }
        for (size_t i = 0; i < paymentDates.size(); ++i)
        {
            double maxSumInsured = getSumInsured(paymentDates[i]);
            double utilisation = min(mtm[i], maxSumInsured);
            utilisation = max(utilisation, 0.0);
            double standby = maxSumInsured - utilisation;
            insurancePayments[i] =
                utilisation * fwdFwdUtilisationFactor[i] + standby * fwdFwdStandbyFactor[i];
        }
    }

    void PortfolioInsurance::runChecks()
    {
        errorTracking->clear();
        if (policyStartDate > policyMaturityDate)
        {
            errorTracking->record(issues::INSURANCE_MATURITY_BEFORE_START,
                                  "Policy maturity is before policy start date",
                                  {"portfolio_insurance", "policy_maturity_date"});
        }

        if ((getSumInsured(policyStartDate) < -1e-14) || (getSumInsured(policyMaturityDate) < -1e-14))
        {
            errorTracking->record(issues::INSURANCE_SUM_NEGATIVE, "Sum insured is negative",
                                  {"portfolio_insurance", "sum_insured"});
        }

        if (!standbyDRS->validate().isOK())
        {
            errorTracking->absorb(standbyDRS->validationReport(),
                                  {"portfolio_insurance", "standby_discount_rate_source"});
        }

        if (!standbyDRS->extendsTo(policyMaturityDate))
        {
            errorTracking->record(issues::INSURANCE_STANDBY_RANGE,
                                  "StandbyDRS does not extend to the policy maturity date",
                                  {"portfolio_insurance", "standby_discount_rate_source"});
        }

        if (!utilisationDRS->validate().isOK())
        {
            errorTracking->absorb(utilisationDRS->validationReport(),
                                  {"portfolio_insurance", "utilisation_discount_rate_source"});
        }

        if (!utilisationDRS->extendsTo(policyMaturityDate))
        {
            errorTracking->record(issues::INSURANCE_UTILISATION_RANGE,
                                  "UtilisationDRS does not extend to the policy maturity date",
                                  {"portfolio_insurance", "utilisation_discount_rate_source"});
        }

        if (standbyDRS->getAnchorDate() != utilisationDRS->getAnchorDate())
        {
            errorTracking->record(issues::INSURANCE_ANCHOR_MISMATCH,
                                  "UtilisationDRS and StandbyDRS do not have the same anchor date",
                                  {"portfolio_insurance"});
        }
        return;
    }

    std::string PortfolioInsurance::validationRoot() const
    {
        return "portfolio_insurance";
    }

    void PortfolioInsurance::checkSizesAndCalcuateSpreads(vector<Date> paymentDates,
                                                          vector<double> mtm,
                                                          vector<double>& insurancePayments)
    {
        if ((paymentDates.size() != mtm.size()) || (paymentDates.size() != insurancePayments.size()))
        {
            insurancePayments = vector<double>(mtm.size(), 0);
            errorTracking->record(issues::INSURANCE_INPUT_DIMENSION,
                                  "Input vectors do not have the same dimension",
                                  {"portfolio_insurance", "inputs"});
            return;
        }
        fwdFwdStandbyFactor.clear();
        fwdFwdStandbyFactor.push_back(1 / standbyDRS->getDiscountFactor(paymentDates[0]) - 1);
        fwdFwdUtilisationFactor.clear();
        fwdFwdUtilisationFactor.push_back(1 / utilisationDRS->getDiscountFactor(paymentDates[0]) - 1);
        for (size_t i = 1; i < paymentDates.size(); ++i)
        {
            fwdFwdStandbyFactor.push_back(
                1 / standbyDRS->getDiscountFactor(paymentDates[i - 1], paymentDates[i]) - 1);
            fwdFwdUtilisationFactor.push_back(
                1 / utilisationDRS->getDiscountFactor(paymentDates[i - 1], paymentDates[i]) - 1);
        }
    }

    /*======================================================================================
   SimpleInsurance: Insurance paid in arrears based on the MtM at the end of the period

    =======================================================================================*/
    SimpleInsurance::SimpleInsurance(Date policyStartDate,
                                     Date policyMaturityDate,
                                     double sumInsuredInput,
                                     std::shared_ptr<DiscountRateSource> standbyDRS,
                                     std::shared_ptr<DiscountRateSource> utilisationDRS)
    {
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

    SimpleInsuranceWithTermStructure::SimpleInsuranceWithTermStructure(
        Date policyStartDate,
        Date policyMaturityDate,
        vector<Date> datesInput,
        vector<double> insuredAmountsInput,
        std::shared_ptr<DiscountRateSource> standbyDRS,
        std::shared_ptr<DiscountRateSource> utilisationDRS)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("SimpleInsuranceWithTermStructure");

        dates = datesInput;
        insuredAmounts = insuredAmountsInput;
        setParameters(policyStartDate, policyMaturityDate, standbyDRS, utilisationDRS);
    }

    void SimpleInsuranceWithTermStructure::runChecks()
    {
        PortfolioInsurance::runChecks();
        if (dates.size() != insuredAmounts.size())
        {
            errorTracking->record(issues::INSURANCE_DATES_AMOUNTS_DIMENSION,
                                  "Dates and insured amounts do not have the same dimension",
                                  {"portfolio_insurance", "insured_amounts"});
        }

        if (dates[0] < policyStartDate)
        {
            errorTracking->record(issues::INSURANCE_FIRST_DATE,
                                  "First insured date preceeds policy start date",
                                  {"portfolio_insurance", "dates"});
        }

        if (dates[dates.size() - 1] != policyMaturityDate)
        {
            errorTracking->record(issues::INSURANCE_LAST_DATE,
                                  "Last insured date must equal policy maturity date",
                                  {"portfolio_insurance", "dates"});
        }

        if (!isStrictlyIncreasing(dates))
        {
            errorTracking->record(issues::INSURANCE_DATES_NOT_INCREASING, "Dates not strictly increasing",
                                  {"portfolio_insurance", "dates"});
        }

        for (size_t i = 0; i < insuredAmounts.size(); ++i)
        {
            if (insuredAmounts[i] < -1e14)
            {
                errorTracking->record(issues::INSURANCE_AMOUNT_NEGATIVE,
                                      "At least one insured amount is negative",
                                      {"portfolio_insurance", "insured_amounts"});
            }
        }
        return;
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
            int ihi = static_cast<int>(dates.size()) - 1;
            int i;
            while (ihi - ilo > 1)
            {
                i = (ihi + ilo) >> 1;
                if (dates[i] >= date)
                    ihi = i;
                else
                    ilo = i;
            }
            SJD_ASSERT(ihi != ilo);
            return insuredAmounts[ihi];
        }
        return 0.0;
    }

}