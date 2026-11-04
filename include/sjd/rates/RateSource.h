#pragma once

#include <set>
#include <vector>

#include <memory>
#include <ql/time/date.hpp>
#include <ql/time/daycounter.hpp>
#include <sjd/tools/Validation.h>

namespace sjd
{

    /**
     * Future-dated rates. Historic fixings are a FixingRateSource, not a RateSource.
     *
     * Nothing is observable before anchorDate. Nothing is stored after finalDate,
     * though a source may still extrapolate past finalDate.
     *
     * Check dates once, at setup. A later get-rate call assumes the date is in
     * range and does not check again, so Monte Carlo does not revalidate every path.
     *
     * If the anchor date has no observation, do not extrapolate back to it.
     * Flatten: every date from the anchor through the first observation uses the
     * first observation's value.
     */
    class RateSource : public Validatable
    {
    public:
        virtual ~RateSource() {};

        void runChecks() override;

        QuantLib::Date getAnchorDate() const;
        QuantLib::Date getFinalDate() const;
        // Use isInRange(..) when you want to include AnchorDate logic
        virtual bool isInRange(QuantLib::Date date) const;
        // Use extendsTo(..) when you want to exclude anchor date logic, most often
        // to understand if you can call the getValue(..) method from on a contract and get a
        // number in return which is the case even if the rate source's anchor date is
        // greater than the contract settlement date (the return value would be zero)
        virtual bool extendsTo(QuantLib::Date date) const;

        bool getAllowsExtrapolation() const;

    protected:
        std::string validationRoot() const override;
        using Validatable::absorbChildReport;
        void absorbChildReport(const ValidationReport& child, const std::string& role);

        QuantLib::Date anchorDate;
        QuantLib::Date finalDate;

        bool allowExtrapolation = false;
        // Set when a constructor flat-fills the anchor from the first observation.
        bool flatBackFilled = false;
    };
}
