#pragma once

#include <memory>
#include <ql/time/date.hpp>
#include <sjd/maths/maths.h>
#include <sjd/tools/Validation.h>
#include <vector>

namespace sjd
{

    /**
     * Historic rate sets. There is no interpolation: a date either has a fixing
     * or it does not. A missing date is an error for the caller, not a rate
     * invented between neighbours.
     */
    class FixingRateSource : public Validatable
    {
    public:
        virtual ~FixingRateSource() {};

        void setDatesAndRates(std::vector<QuantLib::Date> fixingDates, std::vector<double> fixingRates);

        virtual bool isOK();

        // The earliest date which this Rate Source can return a rate
        QuantLib::Date getEarliestDate() const;
        QuantLib::Date getMostRecentDate() const;

        virtual bool containsARateSetOn(QuantLib::Date date) const;
        virtual double getRateSetOn(QuantLib::Date toDate) const;

        // toDates must be strictly increasing
        virtual std::vector<double> getRateSetOn(std::vector<QuantLib::Date> toDates) const;

    protected:
        std::string validationRoot() const override;
        QuantLib::Date firstDate;
        QuantLib::Date lastDate;
        std::vector<QuantLib::Date> fixingDates;
        std::vector<double> fixingRates;
    };

}
