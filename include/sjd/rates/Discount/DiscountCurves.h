#pragma once

#include <sjd/rates/Discount/DeterministicDiscountRateSource.h>
#include <sjd/rates/Discount/InterestRateConvention.h>
#include <sjd/tools/DatedCurve.h>

#include <ql/time/calendars/nullcalendar.hpp>

namespace sjd
{
    /*======================================================================================
     DiscountCurve: The simplest DiscountRateSource

     NOTE : There are no calendar objects here, all dates are assumed to be good dates
             by the time they hit this class
     =======================================================================================*/
    class DiscountCurve : public DeterministicDiscountRateSource
    {
    public:
        DiscountCurve();

        DiscountCurve(QuantLib::Date anchorDate,
                      std::vector<QuantLib::Date> observationDates,
                      std::vector<double> discountFactors,
                      ArrayInterpolatorType type = ArrayInterpolatorType::LINEAR,
                      bool allowExtrapolation = false);

        virtual ~DiscountCurve() {};

        void runChecks() override;
        virtual std::string validationRoot() const { return "discount_curve"; }

        virtual double getDiscountFactor(QuantLib::Date toDate) const;
        using DiscountRateSource::getDiscountFactor;

        std::vector<QuantLib::Date> getObservationDates() const;
        std::vector<double> getDiscountFactors() const;
        ArrayInterpolatorType getType();

        std::vector<std::pair<std::string, std::shared_ptr<RateSource>>> marketStresses() override;
        std::shared_ptr<DiscountRateSource> parallelBump(double spread,
                                                         const InterestRateConvention& irc) override;

        virtual std::shared_ptr<DiscountRateSource> rollForward(QuantLib::Date toDate);

    protected:
        std::shared_ptr<DatedCurve> datedCurve;
        ArrayInterpolatorType type;
    };
}
