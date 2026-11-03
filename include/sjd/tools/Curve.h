#pragma once

#include <memory>
#include <ql/time/date.hpp>
#include <sjd/maths/maths.h>
#include <sjd/tools/Validation.h>

namespace sjd
{
    /*======================================================================================
     Curve:

     NOTE 1) There are no calendar objects here, all dates are assumed to be good dates
             by the time they hit this class
     =======================================================================================*/
    class Curve : public Validatable
    {
    public:
        Curve();

        Curve(QuantLib::Date anchorDate,
              std::vector<double> observationPoints,
              std::vector<double> observedRates,
              ArrayInterpolatorType type = ArrayInterpolatorType::LINEAR,
              bool allowExtrapolation = false);

        void setParameters(QuantLib::Date anchorDate,
                           std::vector<double> observationPoints,
                           std::vector<double> observedRates,
                           ArrayInterpolatorType type = ArrayInterpolatorType::LINEAR,
                           bool allowExtrapolation = false);

        virtual ~Curve() {};

        std::string validationRoot() const override { return "curve"; }

        QuantLib::Date getAnchorDate() const;
        QuantLib::Date getFinalDate();
        ArrayInterpolatorType getType();
        bool getAllowsExtrapolation();

        std::vector<double> getRates();
        void setRates(std::vector<double> updatedRates);

        bool containsRateOnAnchorDate();
        void addPointOnAnchorDate(double valueToAdd);
        double getRate(double toPoint) const;

        std::vector<double> getObservationPoints() const;

    protected:
        void runChecks() override;
        QuantLib::Date anchorDate;
        std::vector<double> observationPoints;
        std::vector<double> observedRates;
        ArrayInterpolatorType type;
        std::shared_ptr<ArrayInterpolator> interpolator;
        bool allowExtrapolation;
    };

}
