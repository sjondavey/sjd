#ifndef SJD_DETERMINISTICVOLATILITYSOURCE_INCLUDED
#define SJD_DETERMINISTICVOLATILITYSOURCE_INCLUDED

#include <sjd/rates/IHaveMarketRateStresses.h>
#include <sjd/rates/Volatility/VolatilityRateSource.h>

namespace sjd
{
    /*======================================================================================
    DeterminisiticVolatilityRateSource

    A deterministic volatility rate source is a rate source with market stresses which allows
    a user to:
        - bump (arithmetically) the source by a fixed volatility at all observation points; and
        - roll the curve to a future date
    =======================================================================================*/
    class DeterministicVolatilityRateSource : public VolatilityRateSource, public IHaveMarketRateStresses
    {
    public:
        virtual bool isOK();

        virtual std::vector<std::pair<std::string, std::shared_ptr<RateSource>>> getMarketRateStresses();
        virtual std::shared_ptr<VolatilityRateSource> parallelBump(double spread) = 0;

        // Roll forward to one date
        virtual std::shared_ptr<VolatilityRateSource> rollForward(QuantLib::Date toDate) = 0;
        // Roll forward to multiple dates. This is a two-step process (std::set dates then roll)
        // provide an interface that is consistent with stochastic rates where the use case
        // is Monte-Carlo simulation (std::set dates once, roll many times)
        virtual void setForwardValuationDates(std::set<QuantLib::Date> dates);
        virtual std::vector<std::shared_ptr<VolatilityRateSource>> rollForward();

    protected:
        std::vector<QuantLib::Date> orderedForwardDates;
    };

}

#endif