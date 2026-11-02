#pragma once

#include <sjd/rates/Forward/ForwardRateSource.h>
#include <sjd/rates/IHaveMarketRateStresses.h>

namespace sjd
{
    /*======================================================================================
    DeterminsitcForwardRateSource

    A deterministic forward rate source is a rate source with market stresses which allows
    a user to:
        - bump the curve by a fixed amount (positive or negative) at all observation points.
            It is up to the user to ensure negative amounts do not make observation points in
            the bumped curve negative because there are no default checks for this; and
        - roll the curve to a future date
    =======================================================================================*/
    class ForwardRateSourceDeterministic : public ForwardRateSource, public IHaveMarketRateStresses
    {
    public:
        virtual bool isOK();

        virtual std::vector<std::pair<std::string, std::shared_ptr<RateSource>>> getMarketRateStresses();
        virtual std::shared_ptr<ForwardRateSource> parallelBump(double spread) = 0;

        // Roll forward to one date
        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate) = 0;
        // For rates environments that have fixing rate sources we need the ability to
        // populate the fixing rates while rolling forward. This is achieved by passing an
        // input std::vector of sorted, increasing rate std::set dates between the anchor date and
        // the “rolledTo” date and getting back the “simulated” rate sets
        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate,
                                                               std::vector<QuantLib::Date> rateSetDates,
                                                               std::vector<double>& rateSetsFromAnchorToDate);
        // Roll forward to multiple dates. This is a two-step process (std::set dates then roll)
        // provide an interface that is consistent with stochastic rates where the use case
        // is Monte-Carlo simulation (std::set dates once, roll many times). The "std::set" structure
        // ensures the dates are ordered and contains no duplicates. The user *must* check
        // all the dates are in an appropriate range by calling the isOK() method after
        // setting these dates.
        virtual void setForwardValuationDates(std::set<QuantLib::Date> dates);
        virtual std::vector<std::shared_ptr<ForwardRateSource>> rollForward();
        // When rolling forward to multiple dates using this method, the input rate std::set dates
        // should be all the rate std::set dates between the anchor date and the *final* date to
        // which the curve will be rolled, i.e. the final “orderedForwardDates”
        virtual std::vector<std::shared_ptr<ForwardRateSource>> rollForward(
            std::vector<QuantLib::Date> rateSetDates, std::vector<double>& rateSetsFromAnchorToFinalRollDate);

    protected:
        std::vector<QuantLib::Date> orderedForwardDates;
    };
}
