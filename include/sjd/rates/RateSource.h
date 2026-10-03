#ifndef SJD_RATESOURCE_INCLUDED
#define SJD_RATESOURCE_INCLUDED
#pragma once

#include <vector>
#include <set>

#include <boost/shared_ptr.hpp>
#include <sjd/tools/Validation.h>
#include <ql/time/date.hpp>
#include <ql/time/daycounter.hpp>

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
    class RateSource 
    {        
    public:
        virtual ~RateSource () {};

        virtual bool isOK();
        void resetErrorMessages();
        std::vector<std::string> getErrorMessages() const;
        std::string getErrorMessagesAsString() const;

        QuantLib::Date getAnchorDate() const;
        QuantLib::Date getFinalDate() const;
        // Use isInRange(..) when you want to include AnchorDate logic
        virtual bool isInRange(QuantLib::Date date) const;
        // Use extendsTo(..) when you want to exclude anchor date logic, most often 
        // to understand if you can call the getValue(..) method from on a contract and get a 
        // number in return which is the case even if the rate source�s anchor date is 
        // greater than the contract settlement date (the return value would be zero)
        virtual bool extendsTo(QuantLib::Date date) const;
        
        bool getAllowsExtrapolation() const;

        virtual ValidationReport validationReport() const;
        virtual ValidationReport validateReport();

    protected:
        virtual std::string validationRoot() const;
        void recordIssue(const std::string& code,
                         const std::string& message,
                         const std::vector<std::string>& tail);
        void absorbChildReport(const ValidationReport& child, const std::string& role);

        boost::shared_ptr<sjd::IssueTracker> errorTracking;

        QuantLib::Date anchorDate;
        QuantLib::Date finalDate;
        
        bool allowExtrapolation;
    };
}

#endif