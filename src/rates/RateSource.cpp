#include <sjd/rates/RateSource.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{

    void RateSource::resetErrorMessages()
    {
        errorTracking->clear();
    }

    std::string RateSource::validationRoot() const
    {
        return "rate_source";
    }

    void RateSource::recordIssue(const std::string& code,
                                 const std::string& message,
                                 const std::vector<std::string>& tail)
    {
        std::vector<std::string> path(1, validationRoot());
        path.insert(path.end(), tail.begin(), tail.end());
        errorTracking->record(code, message, path);
    }

    void RateSource::absorbChildReport(const ValidationReport& child, const std::string& role)
    {
        std::vector<std::string> prefix(1, validationRoot());
        if (!role.empty())
        {
            prefix.push_back(role);
        }
        errorTracking->absorb(child, prefix);
    }

    ValidationReport RateSource::validationReport() const
    {
        return errorTracking->snapshot();
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
        return !errorTracking->getHasErrors();
    }

    vector<string> RateSource::getErrorMessages() const 
    {
        return errorTracking->getErrorMessages();
    }

    string RateSource::getErrorMessagesAsString() const 
    {
        return errorTracking->getErrorMessagesAsString();
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
