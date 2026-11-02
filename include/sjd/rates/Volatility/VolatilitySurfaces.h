#pragma once

#include <sjd/maths/maths.h>
#include <sjd/pricing/Black76Formula.h>
#include <sjd/rates/Volatility/DeterministicVolatilityRateSource.h>

#include <sjd/rates/Forward/ForwardRateSource.h>

#include <ql/math/interpolations/bicubicsplineinterpolation.hpp>
#include <ql/math/interpolations/bilinearinterpolation.hpp>
#include <ql/math/interpolations/interpolation2d.hpp>
#include <ql/math/solvers1d/bisection.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/daycounters/all.hpp>
#include <ql/types.hpp>

enum SurfaceInterpolatorType
{
    BILINEAR,
    BICUBIC
};

namespace sjd
{
    /*======================================================================================
    VolatiltiySurfaceRateSource

    Abstract Base class for volatiltiy surfaces. This class contains basic surface
    functionality

    The wings are flat: a delta or moneyness outside the grid reads the volatility at
    the nearest edge of the grid. allowExtrapolation only controls the time dimension.
    =======================================================================================*/
    class VolatiltiySurfaceRateSource : public DeterministicVolatilityRateSource
    {
    public:
        virtual ~VolatiltiySurfaceRateSource() {};

        virtual bool isOK();
        virtual std::vector<QuantLib::Date> getObservationDates();

        void setInputs(QuantLib::Date anchorDate,
                       std::vector<QuantLib::Date> observationDates,
                       std::vector<double> strikeDimension,
                       QuantLib::Matrix volatilityData,
                       SurfaceInterpolatorType interpolatorType,
                       bool extrapolate);

        virtual SurfaceInterpolatorType getInterpolatorType();
        virtual void setInterpolatorType(SurfaceInterpolatorType interpolatorType);

        virtual void getRolledVariables(QuantLib::Date rolltoDate,
                                        std::vector<QuantLib::Date>& rolledObservationDates,
                                        QuantLib::Matrix& rolledVols);

    protected:
        // time is in the surface's own units: a year fraction, or months for a
        // calendar-month surface.
        bool timeInRange(double time) const;
        double clampToStrikeGrid(double strikeValue) const;
        double surfaceVolatility(double time, double strikeValue) const;

        QuantLib::Matrix convertVectorOfVectorsToMatrix(std::vector<std::vector<double>> volatilityData);
        std::vector<QuantLib::Date> observationDates;
        std::vector<double> time, strikeDimension;
        QuantLib::Matrix volatility;
        std::shared_ptr<QuantLib::Interpolation2D> interpolator;
        SurfaceInterpolatorType interpolatorType;
    };
}
