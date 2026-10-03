#ifndef RATESENVIRONMENT_INCLUDED
#define RATESENVIRONMENT_INCLUDED

#include <cstddef>

#include <sjd/rates/Forward/ForwardRateSource.h>
#include <sjd/rates/Volatility/VolatilityRateSource.h>
#include <sjd/rates/Discount/DiscountRateSource.h>
#include <sjd/rates/Fixing/FixingRateSource.h>
#include <sjd/tools/Validation.h>

namespace sjd 
{
    /**
     * The curves for one single-asset valuation, all sharing this environment's
     * anchor date. The asset is FX, a commodity, or an equity, not an interest rate.
     *
     * The environment may hold curves the contract does not use. A forward does not
     * need a volatility source; an option does.
     *
     * Call contract.validate(environment) and check isOK() before getValue().
     * getValue() does not repeat that check.
     */
    class RatesEnvironment : public RateSource
    {
    public:
        RatesEnvironment(QuantLib::Date anchorDate);

        virtual bool isOK();
        virtual std::string validationRoot() const;

        void setFRS(boost::shared_ptr<ForwardRateSource> frs);
        bool hasFRS();
        boost::shared_ptr<ForwardRateSource> getFRS();

        void setDRS(boost::shared_ptr<DiscountRateSource> drs);
        bool hasDRS();
        boost::shared_ptr<DiscountRateSource> getDRS();

        void setForeignDRS(boost::shared_ptr<DiscountRateSource> foreighDRS);
        bool hasForeignDRS();
        boost::shared_ptr<DiscountRateSource> getForeignDRS();

        void setVRS(boost::shared_ptr<VolatilityRateSource> vrs);
        bool hasVRS();
        boost::shared_ptr<VolatilityRateSource> getVRS();

        void setFixingRS(boost::shared_ptr<FixingRateSource> fixingrs);
        bool hasFixingRS();
        boost::shared_ptr<FixingRateSource> getFixingRS();

        // Advances when a curve pointer on this environment is replaced.
        std::size_t marketRevision() const;

    protected:
        void updateDataAfterSettingRate();

        boost::shared_ptr<ForwardRateSource> frs;
        bool frsSet;
        boost::shared_ptr<VolatilityRateSource> vrs;
        bool vrsSet;
        boost::shared_ptr<DiscountRateSource> drs;
        bool drsSet;
        boost::shared_ptr<DiscountRateSource> foreigndrs;
        bool foreignDrsSet;
        boost::shared_ptr<FixingRateSource> fixingrs;
        bool fixingrsSet;
        std::size_t marketRevision_;
    };
}

#endif