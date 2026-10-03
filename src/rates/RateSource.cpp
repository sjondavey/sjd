#include <sjd/rates/RateSource.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{

    std::string RateSource::validationRoot() const
    {
        return "rate_source";
    }

    void RateSource::absorbChildReport(const ValidationReport& child, const std::string& role)
    {
        absorbChildReport(child, role.empty() ? std::vector<std::string>()
                                              : std::vector<std::string>(1, role));
    }

    ValidationReport RateSource::validateReport()
    {
        isOK();
        return validationReport();
    }

    bool RateSource::isOK()
    {
        resetErrorMessages();
        if (finalDate < anchorDate)  
        {
            recordIssue("RATE_SOURCE_ANCHOR_AFTER_FINAL",
                        "Anchor date after final date",
                        {"anchor_date"});
        }
        return !hasErrors();
    }

    QuantLib::Date RateSource::getAnchorDate() const 
    {
        return anchorDate;
    }

    QuantLib::Date RateSource::getFinalDate() const 
    {
        return finalDate;
    }

    bool RateSource::getAllowsExtrapolation() const 
    {
        return allowExtrapolation;
    }

    bool RateSource::isInRange(QuantLib::Date date) const
    {
        if (allowExtrapolation) 
        {
            return (date >= anchorDate);
        }
        return ((date >= anchorDate) && (date <= finalDate));
    }

    bool RateSource::extendsTo(QuantLib::Date date) const
    {
        if (allowExtrapolation) 
        {
            return true;
        }
        return (date <= finalDate);
    }
}
