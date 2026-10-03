#include <sjd/rates/RatesEnvironment.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{
    RatesEnvironment::RatesEnvironment(Date anchorDateInput)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("RatesEnvironment"));
        anchorDate = anchorDateInput;
        finalDate = anchorDate;
        allowExtrapolation = false;
        frsSet = false;
        vrsSet = false;
        drsSet = false;
        foreignDrsSet = false;
        fixingrsSet = false;
        marketRevision_ = 0;
    }

    bool RatesEnvironment::isOK()
    {
        resetErrorMessages();
        if (frsSet == false &&
            vrsSet == false &&
            drsSet == false &&
            foreignDrsSet == false &&
            fixingrsSet == false)
        {
            recordIssue("RATES_ENVIRONMENT_NO_RATES", "No rates set", {});
            return false;
        }
        if (drsSet)
        {
            if (drs->getAnchorDate() != anchorDate)
            {
                recordIssue("DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH",
                            "Discount rate source does not have the correct anchor date",
                            {"discount_rate_source"});
                return false;
            }
            if (!drs->isOK())
            {
                absorbChildReport(drs->validationReport(), "discount_rate_source");
                return false;
            }
        }
        if (foreignDrsSet)
        {
            if (foreigndrs->getAnchorDate() != anchorDate)
            {
                recordIssue("FOREIGN_DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH",
                            "Foreign discount rate source does not have the correct anchor date",
                            {"foreign_discount_rate_source"});
                return false;
            }
            if (!foreigndrs->isOK())
            {
                absorbChildReport(foreigndrs->validationReport(), "foreign_discount_rate_source");
                return false;
            }
        }
        if (frsSet)
        {
            if (frs->getAnchorDate() != anchorDate)
            {
                recordIssue("FORWARD_RATE_SOURCE_ANCHOR_MISMATCH",
                            "Forward rate source does not have the correct anchor date",
                            {"forward_rate_source"});
                return false;
            }
            if (!frs->isOK())
            {
                absorbChildReport(frs->validationReport(), "forward_rate_source");
                return false;
            }
        }
        if (vrsSet)
        {
            if (vrs->getAnchorDate() != anchorDate)
            {
                recordIssue("VOLATILITY_RATE_SOURCE_ANCHOR_MISMATCH",
                            "Volatility rate source does not have the correct anchor date",
                            {"volatility_rate_source"});
                return false;
            }
            if (!vrs->isOK())
            {
                absorbChildReport(vrs->validationReport(), "volatility_rate_source");
                return false;
            }
        }
        if (fixingrsSet)
        {
            if (!fixingrs->isOK())
            {
                absorbChildReport(fixingrs->validationReport(), "fixing_rate_source");
                return false;
            }
        }
        return !(errorTracking->getHasErrors());
    }

    std::string RatesEnvironment::validationRoot() const
    {
        return "rates_environment";
    }

    void RatesEnvironment::setFRS(std::shared_ptr<ForwardRateSource> frsInput)
    {
        frs = frsInput;
        frsSet = true;
        updateDataAfterSettingRate();
    }

    bool RatesEnvironment::hasFRS()
    {
        return frsSet;
    }

    std::shared_ptr<ForwardRateSource> RatesEnvironment::getFRS()
    {
        return frs;
    }

    void RatesEnvironment::setDRS(std::shared_ptr<DiscountRateSource> drsInput)
    {
        drs = drsInput;
        drsSet = true;
        updateDataAfterSettingRate();
    }

    bool RatesEnvironment::hasDRS()
    {
        return drsSet;
    }

    std::shared_ptr<DiscountRateSource> RatesEnvironment::getDRS()
    {
        return drs;
    }

    void RatesEnvironment::setForeignDRS(std::shared_ptr<DiscountRateSource> foreigndrsInput)
    {
        foreigndrs = foreigndrsInput;
        foreignDrsSet = true;
        updateDataAfterSettingRate();
    }

    bool RatesEnvironment::hasForeignDRS()
    {
        return foreignDrsSet;
    }

    std::shared_ptr<DiscountRateSource> RatesEnvironment::getForeignDRS()
    {
        return foreigndrs;
    }

    void RatesEnvironment::setVRS(std::shared_ptr<VolatilityRateSource> vrsInput)
    {
        vrs = vrsInput;
        vrsSet = true;
        updateDataAfterSettingRate();
    }

    bool RatesEnvironment::hasVRS()
    {
        return vrsSet;
    }

    std::shared_ptr<VolatilityRateSource> RatesEnvironment::getVRS()
    {
        return vrs;
    }

    void RatesEnvironment::setFixingRS(std::shared_ptr<FixingRateSource> fixingrsInput)
    {
        fixingrs = fixingrsInput;
        fixingrsSet = true;
    }

    bool RatesEnvironment::hasFixingRS()
    {
        return fixingrsSet;
    }

    std::shared_ptr<FixingRateSource> RatesEnvironment::getFixingRS()
    {
        return fixingrs;
    }

    std::size_t RatesEnvironment::marketRevision() const
    {
        return marketRevision_;
    }

    void RatesEnvironment::updateDataAfterSettingRate()
    {
        ++marketRevision_;
        allowExtrapolation = true;
        finalDate == anchorDate;
        set<Date> allFinalDates;
        if (drsSet)
        {
            allFinalDates.insert(drs->getFinalDate());
            allowExtrapolation = allowExtrapolation && drs->getAllowsExtrapolation();
        }
        if (foreignDrsSet)
        {
            allFinalDates.insert(foreigndrs->getFinalDate());
            allowExtrapolation = allowExtrapolation && foreigndrs->getAllowsExtrapolation();
        }
        if (frsSet)
        {
            allFinalDates.insert(frs->getFinalDate());
            allowExtrapolation = allowExtrapolation && frs->getAllowsExtrapolation();
        }
        if (vrsSet)
        {
            allFinalDates.insert(vrs->getFinalDate());
            allowExtrapolation = allowExtrapolation && vrs->getAllowsExtrapolation();
        }
        if (allFinalDates.size() > 0)
        {
            finalDate = *allFinalDates.begin();
        }        
    }
}