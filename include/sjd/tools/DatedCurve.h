#pragma once

#include <memory>
#include <ql/time/date.hpp>
#include <sjd/maths/maths.h>

namespace sjd
{
    /*======================================================================================
     DatedCurve:

     NOTE 1) There are no calendar objects here, all dates are assumed to be good dates
             by the time they hit this class
     =======================================================================================*/
    class DatedCurve
    {
    public:
        DatedCurve();

        DatedCurve(QuantLib::Date anchorDate,
                   std::vector<QuantLib::Date> observationDates,
                   std::vector<double> rates,
                   ArrayInterpolatorType type = LINEAR,
                   bool allowExtrapolation = false);

        void setParameters(QuantLib::Date anchorDate,
                           std::vector<QuantLib::Date> observationDates,
                           std::vector<double> rates,
                           ArrayInterpolatorType type = LINEAR,
                           bool allowExtrapolation = false);

        virtual ~DatedCurve() {};

        bool isOK(std::string& errorMessage);

        QuantLib::Date getAnchorDate();
        QuantLib::Date getFinalDate();
        ArrayInterpolatorType getType();
        bool getAllowsExtrapolation();

        std::vector<double> getRates();
        void setRates(std::vector<double> updatedRates);

        bool containsRateOnAnchorDate();
        void addPointOnAnchorDate(double valueToAdd);

        double getRate(QuantLib::Date toDate) const;

        // std::vector<double> getObservationPoints() const;
        std::vector<QuantLib::Date> getObservationDates() const;

    protected:
        QuantLib::Date anchorDate;
        QuantLib::Date finalDate;
        std::vector<QuantLib::Date> observationDates;
        std::vector<double> observationPoints;
        std::vector<double> observedRates;
        ArrayInterpolatorType type;
        std::shared_ptr<ArrayInterpolator> interpolator;
        bool allowExtrapolation;
    };

}
