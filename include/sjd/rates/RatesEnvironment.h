#pragma once

#include <cstddef>

#include <sjd/rates/Discount/DiscountRateSource.h>
#include <sjd/rates/Fixing/FixingRateSource.h>
#include <sjd/rates/Forward/ForwardRateSource.h>
#include <sjd/rates/Volatility/VolatilityRateSource.h>
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

        void runChecks() override;
        virtual std::string validationRoot() const;

        void setFRS(std::shared_ptr<ForwardRateSource> frs);
        bool hasFRS();
        std::shared_ptr<ForwardRateSource> getFRS();

        void setDRS(std::shared_ptr<DiscountRateSource> drs);
        bool hasDRS();
        std::shared_ptr<DiscountRateSource> getDRS();

        void setForeignDRS(std::shared_ptr<DiscountRateSource> foreighDRS);
        bool hasForeignDRS();
        std::shared_ptr<DiscountRateSource> getForeignDRS();

        void setVRS(std::shared_ptr<VolatilityRateSource> vrs);
        bool hasVRS();
        std::shared_ptr<VolatilityRateSource> getVRS();

        void setFixingRS(std::shared_ptr<FixingRateSource> fixingrs);
        bool hasFixingRS();
        std::shared_ptr<FixingRateSource> getFixingRS();

        // Advances when a curve pointer on this environment is replaced.
        std::size_t marketRevision() const;

    protected:
        void updateDataAfterSettingRate();

        std::shared_ptr<ForwardRateSource> frs;
        bool frsSet;
        std::shared_ptr<VolatilityRateSource> vrs;
        bool vrsSet;
        std::shared_ptr<DiscountRateSource> drs;
        bool drsSet;
        std::shared_ptr<DiscountRateSource> foreigndrs;
        bool foreignDrsSet;
        std::shared_ptr<FixingRateSource> fixingrs;
        bool fixingrsSet;
        std::size_t marketRevision_;
    };
}
