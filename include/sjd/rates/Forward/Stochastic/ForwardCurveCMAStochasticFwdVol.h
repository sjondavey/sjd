#ifndef SJD_FUTURESCURVE_INCLUDED
#define SJD_FUTURESCURVE_INCLUDED

#include <sjd/rates/Forward/Deterministic/ForwardCurveCMADeterministic.h>
#include <sjd/rates/Forward/ForwardCurve.h>
#include <sjd/rates/Forward/ForwardRateSourceStochastic.h>

namespace sjd
{
    /*======================================================================================
    ForwardCurveCMAStochasticFwdVol

    QuantLib::Calendar QuantLib::Month Average curve that rolls stochastically
    =======================================================================================*/
    class ForwardCurveCMAStochasticFwdVol : public ForwardRateSourceStochastic
    {
    public:
        // The dates in the observationDates std::vector are only used for their month / year data.
        // If you input two dates in the same month the the class will register an error
        ForwardCurveCMAStochasticFwdVol(QuantLib::Date anchorDate,
                                        std::vector<QuantLib::Date> observationDates,
                                        std::vector<double> forwards,
                                        ArrayInterpolatorType type,
                                        bool allowExtrapolation,
                                        std::shared_ptr<VolatilityRateSource> vrs);

        bool isOK();

        virtual bool isInRange(QuantLib::Date date) const;
        virtual bool extendsTo(QuantLib::Date date) const;

        std::vector<QuantLib::Date> getObservationDates();
        virtual double getForward(QuantLib::Date toDate) const;
        double getForward(QuantLib::Month month, QuantLib::Year year) const;
        virtual std::vector<double> getForward(std::vector<QuantLib::Date> toDate) const;

        std::vector<double> getForwards();
        void setForwards(std::vector<double> updatedForwards);

        virtual void setForwardValuationDates(std::set<QuantLib::Date> dates);
        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate, double normalRV);
        virtual std::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate,
                                                               double normalRV,
                                                               std::vector<QuantLib::Date> rateSetDates,
                                                               std::vector<double>& rateSetsFromAnchorToDate);

        virtual std::vector<std::shared_ptr<ForwardRateSource>> rollForward();
        virtual std::vector<std::shared_ptr<ForwardRateSource>> rollForward(
            std::vector<QuantLib::Date> rateSetDates, std::vector<double>& rateSetsFromAnchorToFinalRollDate);

        virtual std::vector<double> getImpliedRandomNumbers(std::vector<double> spot);

    protected:
        ForwardCurve quotes;
    };
}

#endif