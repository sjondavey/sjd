#pragma once

#include <sjd/rates/Forward/ForwardCurve.h>
#include <sjd/rates/Forward/ForwardRateSourceDeterministic.h>

namespace sjd
{

    /*======================================================================================
    ForwardCurveCMADeterministic

    QuantLib::Calendar QuantLib::Month Average curve that rolls stochastically
    =======================================================================================*/
    class ForwardCurveCMADeterministic : public ForwardRateSourceDeterministic
    {
    public:
        // The dates in the observationDates std::vector are only used for their month / year data.
        // If you input two dates in the same month the the class will register an error
        ForwardCurveCMADeterministic(QuantLib::Date anchorDate,
                                     std::vector<QuantLib::Date> observationDates,
                                     std::vector<double> forwards,
                                     ArrayInterpolatorType type = ArrayInterpolatorType::LINEAR,
                                     bool allowExtrapolation = false);

        void runChecks() override;
        virtual std::string validationRoot() const { return "forward_curve_cma"; }

        virtual bool isInRange(QuantLib::Date date) const;
        virtual bool extendsTo(QuantLib::Date date) const;

        std::vector<QuantLib::Date> getObservationDates();
        virtual double getForward(QuantLib::Date toDate) const;
        double getForward(QuantLib::Month month, QuantLib::Year year) const;
        virtual std::vector<double> getForward(std::vector<QuantLib::Date> toDate) const;
        void setForwards(std::vector<double> updatedForwards);

        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate);
        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate,
                                                               std::vector<QuantLib::Date> rateSetDates,
                                                               std::vector<double>& rateSetsFromAnchorToDate);
        virtual std::vector<std::shared_ptr<ForwardRateSource>> rollForward();
        virtual std::vector<std::shared_ptr<ForwardRateSource>> rollForward(
            std::vector<QuantLib::Date> rateSetDates, std::vector<double>& rateSetsFromAnchorToFinalRollDate);

        virtual std::shared_ptr<ForwardRateSource> parallelBump(double spread);

    protected:
        ForwardCurve quotes;
    };
}
