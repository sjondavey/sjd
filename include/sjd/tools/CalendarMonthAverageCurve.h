#pragma once

#include <boost/math/special_functions/round.hpp>
#include <memory>
#include <ql/time/date.hpp>
#include <sjd/maths/maths.h>

#include <ql/time/calendar.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/simpledaycounter.hpp>

namespace sjd
{
    /*======================================================================================
    CalendarMonthAverageCurve

    =======================================================================================*/
    class CalendarMonthAverageCurve
    {
    public:
        CalendarMonthAverageCurve();

        CalendarMonthAverageCurve(QuantLib::Date anchorDate,
                                  std::vector<QuantLib::Date> observationDates,
                                  std::vector<double> rates,
                                  ArrayInterpolatorType type = ArrayInterpolatorType::LINEAR,
                                  bool allowExtrapolation = false);

        void setParameters(QuantLib::Date anchorDate,
                           std::vector<QuantLib::Date> observationDates,
                           std::vector<double> rates,
                           ArrayInterpolatorType type = ArrayInterpolatorType::LINEAR,
                           bool allowExtrapolation = false);

        bool isOK(std::string& errorMessage);

        QuantLib::Date getAnchorDate() const;
        QuantLib::Date getBeginningOfAnchorDateMonth();
        QuantLib::Date getFinalDate();
        ArrayInterpolatorType getType();
        bool getAllowsExtrapolation();

        std::vector<double> getRates();
        void setRates(std::vector<double> updatedRates);

        bool containsRateOnAnchorDate();
        void addPointOnAnchorDate(double valueToAdd);

        double getRate(QuantLib::Date toDate) const;
        std::vector<double> getRate(std::vector<QuantLib::Date> toDate) const;

        std::vector<double> getObservationPoints() const;
        std::vector<QuantLib::Date> getObservationDates() const;

        static int getNumberOfNumberOfMonthsBetween(QuantLib::Date fromDate, QuantLib::Date toDate);

    protected:
        int getNumberOfMonthsAheadOfAnchorDate(QuantLib::Date date) const;

        QuantLib::Date anchorDate;
        QuantLib::Date beginningOfAnchorDateMonth;
        QuantLib::Date finalDate;
        std::vector<QuantLib::Date> observationDates;
        std::vector<double> observationPoints;
        std::vector<double> observedRates;
        ArrayInterpolatorType type;
        std::shared_ptr<ArrayInterpolator> interpolator;
        bool allowExtrapolation;
        std::shared_ptr<QuantLib::DayCounter> dayCounter;
        std::shared_ptr<QuantLib::DayCounter> a365DayCounter;
    };
}
