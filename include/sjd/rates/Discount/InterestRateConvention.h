#pragma once

#include <memory>
#include <ql/compounding.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/frequency.hpp>

namespace sjd
{
    /*======================================================================================
     InterestRateConvention

     Uses Quantlib structures and encapsulates all the information necessary to use a number
     (double) as an interest rate. The class also contains some basic logic to convert
     interest rates into discount factors and vice-versa
     =======================================================================================*/
    class InterestRateConvention
    {
    public:
        InterestRateConvention(std::shared_ptr<QuantLib::DayCounter> dayCounter,
                               QuantLib::Compounding compounding,
                               QuantLib::Frequency frequency);

        std::shared_ptr<QuantLib::DayCounter> getDayCounter();
        QuantLib::Compounding getCompounding();
        QuantLib::Frequency getFrequency();

        double dfToRate(double df, QuantLib::Date fromDate, QuantLib::Date toDate);

        double rateToDF(double rate, QuantLib::Date fromDate, QuantLib::Date toDate);

    protected:
        std::shared_ptr<QuantLib::DayCounter> dayCounter;
        QuantLib::Compounding compounding;
        QuantLib::Frequency frequency;
    };
}
