#pragma once

#include <boost/math/special_functions/round.hpp>
#include <sjd/rates/Fixing/RollingFixingRateSource.h>

namespace sjd
{

    /*======================================================================================
    DailyFixingCurve

    The simplest fixing curve which contains one fixing rate per date
    =======================================================================================*/
    class DailyFixingCurve : public RollingFixingRateSource
    {
    public:
        DailyFixingCurve(std::vector<QuantLib::Date> fixingDates, std::vector<double> fixingRates);

        virtual std::string validationRoot() const { return "fixing_curve"; }

        virtual std::shared_ptr<RollingFixingRateSource> rollForward(
            QuantLib::Date toDate,
            std::vector<QuantLib::Date> newRateSetDates,
            std::vector<double> newRateSets);
    };

    /*======================================================================================
    MonthlyFixingCurve

     A Fixing Curve where there is only one point per month but this point represents the
     (cumulative) month average
     =======================================================================================*/
    class MonthlyFixingCurve : public RollingFixingRateSource
    {
    public:
        // The dates in the observationDates std::vector are only used for their month / year data.
        // If you input two dates in the same month the the class will register an error
        MonthlyFixingCurve(std::vector<QuantLib::Date> fixingDates, std::vector<double> fixingRates);

        ~MonthlyFixingCurve();

        virtual std::shared_ptr<RollingFixingRateSource> rollForward(
            QuantLib::Date toDate,
            std::vector<QuantLib::Date> newRateSetDates,
            std::vector<double> newRateSets);

        virtual std::vector<QuantLib::Date> getFixingDatesFor(std::vector<QuantLib::Date> inputDates);
        // fixing dates are the 1st of the month. There is a chance that the last forward
        // fixing curve will be the same as the first appened fixing date (since they are
        // all std::set to the 1st of the month)
        virtual void appendRateSets(std::vector<QuantLib::Date> fixingDates, std::vector<double> fixingRates);

        virtual bool containsARateSetOn(QuantLib::Date date) const;
        virtual double getRateSetOn(QuantLib::Date toDate) const;
        virtual std::vector<double> getRateSetOn(std::vector<QuantLib::Date> toDates) const;
    };

    /*======================================================================================
     HistoricFedFundsCurve

     Used to determine the interest payable on a margin account
     =======================================================================================*/
    class HistoricFedFundsCurve : public DailyFixingCurve
    {
    public:
        HistoricFedFundsCurve(std::vector<QuantLib::Date> fixingDates, std::vector<double> fixingRates);
        virtual ~HistoricFedFundsCurve();

        void runChecks() override;

        bool canCalculateInterestFor(QuantLib::Date startDate,
                                     QuantLib::Date endDate,
                                     std::vector<QuantLib::Date> marginCallDates,
                                     std::vector<double> marginCallAmounts);

        double calculateNonCompoundedInterestOnMargin(double openingBalance,
                                                      QuantLib::Date startDate,
                                                      QuantLib::Date endDate,
                                                      std::vector<QuantLib::Date> marginCallDates,
                                                      std::vector<double> marginCallAmounts);
    };

}
