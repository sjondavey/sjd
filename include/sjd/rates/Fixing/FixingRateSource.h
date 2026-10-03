#ifndef SJD_FIXINGRATESOURCE_INCLUDED
#define SJD_FIXINGRATESOURCE_INCLUDED
#pragma once

#include <vector>
#include <ql/time/date.hpp>
#include <sjd/tools/Validation.h>
#include <sjd/maths/maths.h>
#include <boost/shared_ptr.hpp>

namespace sjd 
{

   /**
    * Historic rate sets. There is no interpolation: a date either has a fixing
    * or it does not. A missing date is an error for the caller, not a rate
    * invented between neighbours.
    */
    class FixingRateSource
    {
    public:
        virtual ~FixingRateSource() {};

        void setDatesAndRates(std::vector<QuantLib::Date> fixingDates, std::vector<double> fixingRates);

        virtual bool isOK();
        std::vector<std::string> getErrorMessages() const;
        ValidationReport validationReport() const;

        // The earliest date which this Rate Source can return a rate
        QuantLib::Date getEarliestDate() const;
        QuantLib::Date getMostRecentDate() const;

        virtual bool containsARateSetOn(QuantLib::Date date) const;
        virtual double getRateSetOn(QuantLib::Date toDate) const;

        // toDates must be strictly increasing
        virtual std::vector<double> getRateSetOn(std::vector<QuantLib::Date> toDates) const;

        boost::shared_ptr<sjd::IssueTracker> errorTracking;

    protected:
        virtual std::string validationRoot() const;
        void recordIssue(const std::string& code,
                         const std::string& message,
                         const std::vector<std::string>& path);
        QuantLib::Date firstDate;
        QuantLib::Date lastDate;
        std::vector<QuantLib::Date> fixingDates;
        std::vector<double> fixingRates;
    };

}

#endif