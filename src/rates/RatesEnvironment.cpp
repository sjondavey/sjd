#include <sjd/rates/RatesEnvironment.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    RatesEnvironment::RatesEnvironment(Date anchorDateInput)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("RatesEnvironment");
        anchorDate = anchorDateInput;
        finalDate = anchorDate;
        allowExtrapolation = false;
        marketRevision_ = 0;
    }

    void RatesEnvironment::runChecks()
    {
        resetErrorMessages();
        if (!frs && !vrs && !drs && !foreigndrs && !fixingrs)
        {
            recordIssue(issues::RATES_ENVIRONMENT_NO_RATES, "No rates set", {});
            return;
        }
        if (drs)
        {
            if (drs->getAnchorDate() != anchorDate)
            {
                recordIssue(issues::DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH,
                            "Discount rate source does not have the correct anchor date",
                            {"discount_rate_source"});
                return;
            }
            if (!drs->validate().isOK())
            {
                absorbChildReport(drs->validationReport(), "discount_rate_source");
                return;
            }
        }
        if (foreigndrs)
        {
            if (foreigndrs->getAnchorDate() != anchorDate)
            {
                recordIssue(issues::FOREIGN_DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH,
                            "Foreign discount rate source does not have the correct anchor date",
                            {"foreign_discount_rate_source"});
                return;
            }
            if (!foreigndrs->validate().isOK())
            {
                absorbChildReport(foreigndrs->validationReport(), "foreign_discount_rate_source");
                return;
            }
        }
        if (frs)
        {
            if (frs->getAnchorDate() != anchorDate)
            {
                recordIssue(issues::FORWARD_RATE_SOURCE_ANCHOR_MISMATCH,
                            "Forward rate source does not have the correct anchor date",
                            {"forward_rate_source"});
                return;
            }
            if (!frs->validate().isOK())
            {
                absorbChildReport(frs->validationReport(), "forward_rate_source");
                return;
            }
        }
        if (vrs)
        {
            if (vrs->getAnchorDate() != anchorDate)
            {
                recordIssue(issues::VOLATILITY_RATE_SOURCE_ANCHOR_MISMATCH,
                            "Volatility rate source does not have the correct anchor date",
                            {"volatility_rate_source"});
                return;
            }
            if (!vrs->validate().isOK())
            {
                absorbChildReport(vrs->validationReport(), "volatility_rate_source");
                return;
            }
        }
        if (fixingrs)
        {
            if (!fixingrs->validate().isOK())
            {
                absorbChildReport(fixingrs->validationReport(), "fixing_rate_source");
                return;
            }
        }
        return;
    }

    std::string RatesEnvironment::validationRoot() const
    {
        return "rates_environment";
    }

    void RatesEnvironment::setFRS(const std::shared_ptr<ForwardRateSource>& frsInput)
    {
        frs = frsInput;
        updateDataAfterSettingRate();
    }

    bool RatesEnvironment::hasFRS() const
    {
        return static_cast<bool>(frs);
    }

    std::shared_ptr<ForwardRateSource> RatesEnvironment::getFRS()
    {
        return frs;
    }

    void RatesEnvironment::setDRS(const std::shared_ptr<DiscountRateSource>& drsInput)
    {
        drs = drsInput;
        updateDataAfterSettingRate();
    }

    bool RatesEnvironment::hasDRS() const
    {
        return static_cast<bool>(drs);
    }

    std::shared_ptr<DiscountRateSource> RatesEnvironment::getDRS()
    {
        return drs;
    }

    void RatesEnvironment::setForeignDRS(const std::shared_ptr<DiscountRateSource>& foreigndrsInput)
    {
        foreigndrs = foreigndrsInput;
        updateDataAfterSettingRate();
    }

    bool RatesEnvironment::hasForeignDRS() const
    {
        return static_cast<bool>(foreigndrs);
    }

    std::shared_ptr<DiscountRateSource> RatesEnvironment::getForeignDRS()
    {
        return foreigndrs;
    }

    void RatesEnvironment::setVRS(const std::shared_ptr<VolatilityRateSource>& vrsInput)
    {
        vrs = vrsInput;
        updateDataAfterSettingRate();
    }

    bool RatesEnvironment::hasVRS() const
    {
        return static_cast<bool>(vrs);
    }

    std::shared_ptr<VolatilityRateSource> RatesEnvironment::getVRS()
    {
        return vrs;
    }

    void RatesEnvironment::setFixingRS(const std::shared_ptr<FixingRateSource>& fixingrsInput)
    {
        fixingrs = fixingrsInput;
        ++marketRevision_;
    }

    bool RatesEnvironment::hasFixingRS() const
    {
        return static_cast<bool>(fixingrs);
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
        finalDate = anchorDate;
        set<Date> allFinalDates;
        if (drs)
        {
            allFinalDates.insert(drs->getFinalDate());
            allowExtrapolation = allowExtrapolation && drs->getAllowsExtrapolation();
        }
        if (foreigndrs)
        {
            allFinalDates.insert(foreigndrs->getFinalDate());
            allowExtrapolation = allowExtrapolation && foreigndrs->getAllowsExtrapolation();
        }
        if (frs)
        {
            allFinalDates.insert(frs->getFinalDate());
            allowExtrapolation = allowExtrapolation && frs->getAllowsExtrapolation();
        }
        if (vrs)
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