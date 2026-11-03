#include <sjd/rates/Volatility/VolatilitySurfaceDelta.h>

#include <sjd/tools/Errors.h>

#include <boost/algorithm/string.hpp>
#include <sjd/tools/Errors.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    VolatilitySurfaceDelta

    =======================================================================================*/
    VolatilitySurfaceDelta::VolatilitySurfaceDelta(Date anchorDateInput,
                                                   vector<Date> observationDatesInput,
                                                   vector<double> deltaInput,
                                                   vector<vector<double>> volatilityInput,
                                                   SurfaceInterpolatorType interpolatorTypeInput,
                                                   bool extrapolateInput)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("VolatilitySurfaceDelta");

        setInputs(anchorDateInput, observationDatesInput, deltaInput,
                  convertVectorOfVectorsToMatrix(volatilityInput), interpolatorTypeInput, extrapolateInput);
    }

    VolatilitySurfaceDelta::VolatilitySurfaceDelta(Date anchorDateInput,
                                                   vector<Date> observationDatesInput,
                                                   vector<double> deltaInput,
                                                   QuantLib::Matrix volatilityInput,
                                                   SurfaceInterpolatorType interpolatorTypeInput,
                                                   bool extrapolateInput)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("VolatilitySurfaceDelta");

        setInputs(anchorDateInput, observationDatesInput, deltaInput, volatilityInput, interpolatorTypeInput,
                  extrapolateInput);
    }

    void VolatilitySurfaceDelta::runChecks()
    {
        VolatiltiySurfaceRateSource::runChecks();
        return;
    }

    bool VolatilitySurfaceDelta::isInRange(QuantLib::Date date, double /*strike*/, double /*forward*/) const
    {
        return isInRange(date);
    }

    bool VolatilitySurfaceDelta::isInRange(QuantLib::Date date) const
    {
        return timeInRange(dayCounter->yearFraction(anchorDate, date));
    }

    double VolatilitySurfaceDelta::getVolatility(Date toDate) const
    {
        return getVolatility(toDate, 1.0, 1.0);
    }

    double VolatilitySurfaceDelta::getVolatility(Date toDate, double strike, double forward) const
    {
        double delta = calculateDeltaFromStrike(toDate, strike, forward);
        double yf = dayCounter->yearFraction(anchorDate, toDate);
        return surfaceVolatility(yf, delta);
    }

    double VolatilitySurfaceDelta::getStandardDeviation(Date toDate) const
    {
        return getStandardDeviation(toDate, 1.0, 1.0);
    };

    double VolatilitySurfaceDelta::getStandardDeviation(Date toDate, double strike, double forward) const
    {
        double vol = getVolatility(toDate, strike, forward);
        double yf = dayCounter->yearFraction(anchorDate, toDate);
        return vol * sqrt(yf);
    }

    double VolatilitySurfaceDelta::calculateDeltaFromStrike(Date date, double strike, double forward) const
    {
        double yf = dayCounter->yearFraction(anchorDate, date);
        if (yf <= 0.0)
        {
            // With no time left the put delta is a step at the strike.
            if (strike > forward)
                return 100.0;
            if (strike < forward)
                return 0.0;
            return 50.0;
        }
        double accuracy = 1.0e-8;
        size_t maxIterations = 20;
        double sqrtttm = sqrt(yf);

        double guess1 = 50, guess2 = 50;
        double vol1, sd1 = 0.2, diff = accuracy + 1;

        Black76Put put(forward, strike, sd1, 1);

        size_t counter = 0;

        while ((counter < maxIterations) && (diff > accuracy))
        {
            guess1 = guess2;
            try
            {
                vol1 = (*interpolator)(yf, clampToStrikeGrid(guess1), true); // don't want to fall over here
            }
            catch (const std::exception&)
            {
                throw NumericalError("Could not convert a strike to a delta on the volatility surface");
            }
            sd1 = vol1 * sqrtttm;
            put.setStandardDeviation(sd1);
            guess2 = -put.getDelta() * 100;
            diff = abs(guess1 - guess2);
            ++counter;
        }
        return guess2;
    }

    double VolatilitySurfaceDelta::calculateStrikeFromDelta(Date date, double delta, double forward) const
    {
        QuantLib::Real accuracy = 1.0e-12;
        QuantLib::Bisection solver;
        double yf = dayCounter->yearFraction(anchorDate, date);
        double vol = surfaceVolatility(yf, delta);
        double sd = vol * sqrt(yf);
        double strike;

        double guess = forward; // frs->getForward(date);
        double lowerBound = 0.1 * guess;
        double upperBound = 2.0 * guess;
        try
        {
            strike = solver.solve(BlackPutImpliedStrike(delta, forward, sd), accuracy, guess, lowerBound,
                                  upperBound);
        }
        catch (const std::exception&)
        {
            throw NumericalError("Could not convert a delta to a strike on the volatility surface");
        }
        return strike;
    }

    std::shared_ptr<VolatilityRateSource> VolatilitySurfaceDelta::parallelBump(double spread)
    {
        vector<Date> newDates;
        QuantLib::Matrix volatilitySubset;

        getRolledVariables(anchorDate, newDates, volatilitySubset);
        Matrix spreadMatrix = Matrix(volatilitySubset.rows(), volatilitySubset.columns(), spread);
        Matrix bumpedVolMatrix = volatilitySubset + spreadMatrix;

        return std::make_shared<VolatilitySurfaceDelta>(
            anchorDate, newDates, strikeDimension, volatilitySubset, interpolatorType, allowExtrapolation);
    }

    std::shared_ptr<VolatilityRateSource> VolatilitySurfaceDelta::rollForward(Date toDate)
    {
        vector<Date> newDates;
        QuantLib::Matrix volatilitySubset;

        getRolledVariables(toDate, newDates, volatilitySubset);

        return std::make_shared<VolatilitySurfaceDelta>(toDate, newDates, strikeDimension, volatilitySubset,
                                                        interpolatorType, allowExtrapolation);
    }

    /*======================================================================================
    SimpleDeltaSurface

    =======================================================================================*/
    SimpleDeltaSurface::SimpleDeltaSurface(vector<double> timesInput,
                                           vector<double> deltaInput,
                                           vector<vector<double>> volatility,
                                           bool extrapolate,
                                           string interpolationType)
        : extrapolate(extrapolate), times(timesInput), delta(deltaInput), rows(volatility),
          interpolationType(interpolationType)
    {
        errorTracking = std::make_shared<IssueTracker>("SimpleDeltaSurface");
        boost::algorithm::to_lower(this->interpolationType);
        boost::algorithm::trim(this->interpolationType);
        if (!validate().isOK())
            return;
        if (delta[0] < 1.0)
        {
            for (size_t i = 0; i < delta.size(); ++i)
            {
                delta[i] *= 100;
            }
        }
        if (times[0] > 0)
        {
            times.insert(times.begin(), 0);
            for (size_t i = 0; i < rows.size(); ++i)
            {
                double vol = rows[i][0];
                rows[i].insert(rows[i].begin(), vol);
            }
        }
        const size_t rowCount = rows.size();
        const size_t columnCount = rows[0].size();
        volatilityMatrix = Matrix(rowCount, columnCount);
        for (size_t i = 0; i < rowCount; ++i)
        {
            for (size_t j = 0; j < columnCount; ++j)
            {
                volatilityMatrix[i][j] = rows[i][j];
            }
        }
        if (this->interpolationType.compare("bilinear") == 0)
        {
            interpolator = std::make_shared<BilinearInterpolation>(times.begin(), times.end(), delta.begin(),
                                                                   delta.end(), volatilityMatrix);
        }
        else
        {
            interpolator = std::make_shared<BicubicSpline>(times.begin(), times.end(), delta.begin(),
                                                           delta.end(), volatilityMatrix);
        }
    }

    void SimpleDeltaSurface::runChecks()
    {
        if (!((interpolationType.compare("bilinear") == 0) || (interpolationType.compare("bicubic") == 0)))
        {
            recordIssue(issues::VOLATILITY_SURFACE_INTERPOLATOR_TYPE,
                        "Interpolation Type must be either Bilinear or Bicubic", {});
            return;
        }
        if ((times.size() == 0) || (delta.size() == 0) || (rows.size() == 0))
        {
            recordIssue(issues::VOLATILITY_SURFACE_INPUTS_EMPTY, "One of the input vectors is empty", {});
            return;
        }
        const size_t columns = rows[0].size();
        for (size_t i = 0; i < rows.size(); ++i)
        {
            if (rows[i].size() != columns)
            {
                recordIssue(issues::VOLATILITY_SURFACE_DIMENSION_MISMATCH,
                            "The input volatility is not square", {});
                return;
            }
        }
        if (!isStrictlyIncreasing<double>(times))
        {
            recordIssue(issues::VOLATILITY_SURFACE_TIMES_NOT_INCREASING, "Times are not strictly increasing",
                        {});
            return;
        }
        if (!isStrictlyIncreasing<double>(delta))
        {
            recordIssue(issues::VOLATILITY_SURFACE_STRIKES_NOT_INCREASING,
                        "Deltas are not strictly increasing", {});
        }
    }

    bool SimpleDeltaSurface::isInDeltaRange(double time, double deltaPoint)
    {
        if ((extrapolate) || interpolator->isInRange(time, deltaPoint))
        {
            return true;
        }
        return false;
    }

    bool SimpleDeltaSurface::isInMoneynessRange(double time, double moneyness)
    {
        if (extrapolate)
        {
            return true;
        }
        double fwd = 1;
        double strike = moneyness + fwd;
        double vol = calculateDeltaFromStrike(fwd, strike, time);
        if (abs(vol) < 1e-8)
        {
            return false;
        }
        return true;
    }

    double SimpleDeltaSurface::getVolatility(double time) const
    {
        double putDelta = calculateDeltaFromStrike(1, 1, time);
        return getVolatilityForDelta(time, putDelta);
    }

    double SimpleDeltaSurface::getVolatilityForDelta(double time, double deltaPoint) const
    {
        SJD_ASSERT(interpolator && (extrapolate || interpolator->isInRange(time, deltaPoint)));
        return (*interpolator)(time, deltaPoint, extrapolate);
    }

    double SimpleDeltaSurface::getVolatilityForMoneyness(double time, double moneyness) const
    {
        if (time <= 0)
        {
            return 0;
        }
        double fwd = 1;
        double strike = moneyness + fwd;
        double putDelta = calculateDeltaFromStrike(fwd, strike, time);
        return getVolatilityForDelta(time, putDelta);
    }

    // will return 0 if outside the interpolation range
    double SimpleDeltaSurface::calculateDeltaFromStrike(double forward, double strike, double time) const
    {
        double accuracy = 1.0e-8;
        size_t maxIterations = 20;
        double sqrtttm = sqrt(time);
        double guess1 = 50, guess2 = 50;
        double vol1 = 0, sd1 = 0.2, diff = accuracy + 1;
        Black76Put put(forward, strike, sd1, 1);
        size_t counter = 0;
        while ((counter < maxIterations) && (diff > accuracy))
        {
            guess1 = guess2;
            if (interpolator->isInRange(time, guess1))
            {
                vol1 = (*interpolator)(time, guess1, extrapolate);
            }
            sd1 = vol1 * sqrtttm;
            put.setStandardDeviation(sd1);
            guess2 = -put.getDelta() * 100;
            diff = abs(guess1 - guess2);
            ++counter;
        }
        return guess2;
    }
}