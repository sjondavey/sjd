#ifndef SJD_ROLLINGFIXINGRATESOURCE_INCLUDED
#define SJD_ROLLINGFIXINGRATESOURCE_INCLUDED
#pragma once

#include <sjd/rates/Fixing/FixingRateSource.h>

namespace sjd
{

    /*======================================================================================
    FixingRateSource

    A curve that contains historic rate sets.

    A fixing curve must NOT interpolate, either there is a rate for a date or there is not
    if there is not a rate the calling class should be notified
    =======================================================================================*/
    class RollingFixingRateSource : public FixingRateSource
    {
    public:
        virtual ~RollingFixingRateSource() {};

        virtual std::shared_ptr<RollingFixingRateSource> rollForward(
            QuantLib::Date toDate,
            std::vector<QuantLib::Date> newRateSetDates,
            std::vector<double> newRateSets) = 0;

        // QuantLib::Calendar month average fixes may only store fixing rates on a subset of the all
        // the input dates
        virtual std::vector<QuantLib::Date> getFixingDatesFor(std::vector<QuantLib::Date> inputDates);
        void captureOriginalState();
        // Ensure that appended fixing rates are after the last observation date first
        virtual void appendRateSets(std::vector<QuantLib::Date> fixingDates, std::vector<double> fixingRates);
        virtual void updateAppedededRates(std::vector<double> fixingRates);
        void reset();

    protected:
        QuantLib::Date appendedLastDate;
        std::vector<QuantLib::Date> originalDates;
        std::vector<double> originalFixes;
        QuantLib::Date originalFinalDate;

    private:
    };

}

#endif