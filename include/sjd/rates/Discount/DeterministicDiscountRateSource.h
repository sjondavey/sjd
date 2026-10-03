#ifndef SJD_DETERMINISTICDISCOUNTRATESOURCE_INCLUDED
#define SJD_DETERMINISTICDISCOUNTRATESOURCE_INCLUDED

#include <sjd/rates/Discount/DiscountRateSource.h>
#include <ql/time/daycounters/actual360.hpp>
#include <sjd/rates/IHaveMarketRateStresses.h>
#include <sjd/rates/Discount/InterestRateConvention.h>

namespace sjd 
{

    /*======================================================================================
    DeterminisiticRateSource 
    
    A deterministic discount rate source is a rate source with market stresses which allows 
    a user to:
        - bump the curve by a fixed interest rate at all observation points; and 
        - roll the curve to a future date by preserving the forward forward discount factors
    =======================================================================================*/
    class DeterministicDiscountRateSource : public DiscountRateSource, 
                                            public IHaveMarketRateStresses
    {
    public:
        virtual bool isOK();

        virtual std::vector<std::pair<std::string, std::shared_ptr<RateSource>>> getMarketRateStresses();
        virtual std::shared_ptr<DiscountRateSource> parallelBump(double spread,
                                                                   std::shared_ptr<InterestRateConvention> irc) = 0;

        // Roll forward to one date
        virtual std::shared_ptr<DiscountRateSource> rollForward(QuantLib::Date toDate) = 0;
        // Roll forward to multiple dates. This is a two-step process (std::set dates then roll) 
        // provide an interface that is consistent with stochastic rates where the use case 
        // is Monte-Carlo simulation (std::set dates once, roll many times). The "std::set" structure 
        // ensures the dates are ordered and contains no duplicates. The user *must* check 
        // all the dates are in an appropriate range by calling the isOK() method after 
        // setting these dates.
        virtual void setForwardValuationDates(std::set<QuantLib::Date> dates); 
        virtual std::vector<std::shared_ptr<DiscountRateSource> > rollForward();

    protected:
        std::vector<QuantLib::Date> orderedForwardDates;
    };

}

#endif