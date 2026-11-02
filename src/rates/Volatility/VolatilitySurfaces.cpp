#include <sjd/rates/Volatility/VolatilitySurfaces.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
   VolatiltiySurfaceRateSource

    =======================================================================================*/
    bool VolatiltiySurfaceRateSource::isOK()
    {
        DeterministicVolatilityRateSource::isOK();

        if ((observationDates.size() == 0) || (strikeDimension.size() == 0) || (volatility.empty()))
        {
            recordIssue("VOLATILITY_SURFACE_INPUTS_EMPTY",
                        "One of observation dates, moneyness or volatility is empty", {"inputs"});
        }
        if ((observationDates.size() != volatility.columns()) ||
            (strikeDimension.size() != volatility.rows()))
        {
            recordIssue("VOLATILITY_SURFACE_DIMENSION_MISMATCH",
                        "Input vectors and matricies have inconsistent dimension", {"inputs"});
        }
        if (!isStrictlyIncreasing<Date>(observationDates))
        {
            recordIssue("VOLATILITY_SURFACE_DATES_NOT_INCREASING", "Input dates not strictly increasing",
                        {"observation_dates"});
        }
        if (!isStrictlyIncreasing<double>(time))
        {
            recordIssue("VOLATILITY_SURFACE_TIMES_NOT_INCREASING", "Input times not strictly increasing",
                        {"times"});
        }
        if (!isStrictlyIncreasing<double>(strikeDimension))
        {
            recordIssue("VOLATILITY_SURFACE_STRIKES_NOT_INCREASING",
                        "Strike dimension inputs are not strictly increasing", {"strikes"});
        }
        if ((interpolatorType != BILINEAR) && (interpolatorType != BICUBIC))
        {
            recordIssue("VOLATILITY_SURFACE_INTERPOLATOR_TYPE",
                        "Interpolator Type must be either Bilinear or Bicubic", {"interpolator"});
        }
        if (interpolator == NULL)
        {
            recordIssue("VOLATILITY_SURFACE_INTERPOLATOR_MISSING", "Interpolator not set", {"interpolator"});
        }
        if (finalDate < anchorDate)
        {
            recordIssue("VOLATILITY_SURFACE_FINAL_BEFORE_ANCHOR", "Final date before anchor date",
                        {"final_date"});
        }
        return !errorTracking->getHasErrors();
    }

    vector<Date> VolatiltiySurfaceRateSource::getObservationDates()
    {
        return observationDates;
    }

    bool VolatiltiySurfaceRateSource::timeInRange(double t) const
    {
        if (time.empty())
        {
            return false;
        }
        if (allowExtrapolation)
        {
            return t >= time.front();
        }
        return (t >= time.front()) && (t <= time.back());
    }

    double VolatiltiySurfaceRateSource::clampToStrikeGrid(double strikeValue) const
    {
        if (strikeDimension.empty())
        {
            return strikeValue;
        }
        return std::min(std::max(strikeValue, strikeDimension.front()), strikeDimension.back());
    }

    double VolatiltiySurfaceRateSource::surfaceVolatility(double t, double strikeValue) const
    {
        return (*interpolator)(t, clampToStrikeGrid(strikeValue), allowExtrapolation);
    }

    void VolatiltiySurfaceRateSource::setInputs(Date anchorDateInput,
                                                vector<Date> observationDatesInput,
                                                vector<double> strikeDimensionInput,
                                                QuantLib::Matrix volatilityInput,
                                                SurfaceInterpolatorType interpolatorTypeInput,
                                                bool extrapolateInput)
    {
        if (observationDatesInput.size() == 0 || strikeDimensionInput.size() == 0 ||
            volatilityInput.rows() * volatilityInput.columns() == 0)
        {
            observationDates = vector<Date>();
            strikeDimension = vector<double>();
            volatility = Matrix(0, 0);
            return;
        }
        anchorDate = anchorDateInput;
        dayCounter = std::make_shared<Actual365Fixed>();
        allowExtrapolation = extrapolateInput;

        strikeDimension = strikeDimensionInput;
        observationDates = observationDatesInput;
        if (observationDates[0] > anchorDate)
        {
            observationDates.insert(observationDates.begin(), anchorDate);
            volatility = QuantLib::Matrix(volatilityInput.rows(), volatilityInput.columns() + 1);
            for (size_t i = 0; i < volatilityInput.rows(); ++i)
            {
                volatility[i][0] = volatilityInput[i][0];
                for (size_t j = 0; j < volatilityInput.columns(); ++j)
                {
                    volatility[i][j + 1] = volatilityInput[i][j];
                }
            }
        }
        else
        {
            volatility = volatilityInput;
        }

        time.clear();
        for (size_t i = 0; i < observationDates.size(); ++i)
        {
            double yf = dayCounter->yearFraction(anchorDate, observationDates[i]);
            time.push_back(yf);
        }

        if (observationDates.size() > 0)
        {
            finalDate = observationDates.back();
        }
        else
        {
            finalDate = Date(01, Jan, 1901);
        }
        setInterpolatorType(interpolatorTypeInput);
    }

    SurfaceInterpolatorType VolatiltiySurfaceRateSource::getInterpolatorType()
    {
        return interpolatorType;
    }

    void VolatiltiySurfaceRateSource::setInterpolatorType(SurfaceInterpolatorType interpolatorTypeInput)
    {
        if ((time.size() == 0) || (strikeDimension.size() == 0) ||
            (volatility.rows() * volatility.columns() == 0))
        {
            return;
        }
        interpolatorType = interpolatorTypeInput;
        if (interpolatorType == BILINEAR)
        {
            interpolator = std::make_shared<QuantLib::BilinearInterpolation>(
                time.begin(), time.end(), strikeDimension.begin(), strikeDimension.end(), volatility);
        }
        else if (interpolatorType == BICUBIC)
        {
            interpolator = std::make_shared<QuantLib::BicubicSpline>(
                time.begin(), time.end(), strikeDimension.begin(), strikeDimension.end(), volatility);
        }
        else
        {
            interpolator = std::make_shared<QuantLib::BicubicSpline>(
                time.begin(), time.end(), strikeDimension.begin(), strikeDimension.end(), volatility);
            interpolatorType = BICUBIC;
        }
    }

    void VolatiltiySurfaceRateSource::getRolledVariables(Date toDate,
                                                         vector<Date>& newDates,
                                                         QuantLib::Matrix& volatilitySubset)
    {
        if (!isInRange(toDate))
        {
            newDates.clear();
            newDates.push_back(toDate);
            volatilitySubset = QuantLib::Matrix(0, 0);
            return;
        }
        else
        {
            size_t offset = 0;
            for (size_t i = 0; i < observationDates.size(); ++i)
            {
                double yearFraction = dayCounter->yearFraction(toDate, observationDates[i]);
                if (yearFraction >= 0)
                {
                    newDates.push_back(observationDates[i]);
                }
                else
                {
                    ++offset;
                }
            }
            volatilitySubset = QuantLib::Matrix(strikeDimension.size(), newDates.size());
            for (size_t i = 0; i < strikeDimension.size(); ++i)
            {
                for (size_t j = 0; j < newDates.size(); ++j)
                {
                    volatilitySubset[i][j] = volatility[i][j + offset];
                }
            }
            return;
        }
    }

    QuantLib::Matrix VolatiltiySurfaceRateSource::convertVectorOfVectorsToMatrix(
        vector<vector<double>> volatilityData)
    {
        if (volatilityData.size() == 0)
        {
            return Matrix(1, 1, 0);
        }
        else
        {
            size_t rows, columns;
            rows = volatilityData.size();
            columns = volatilityData[0].size();
            Matrix M = Matrix(rows, columns);
            for (size_t i = 0; i < rows; ++i)
            {
                if (columns != volatilityData[i].size())
                {
                    return Matrix(1, 1, 0);
                    ;
                }
                for (size_t j = 0; j < columns; ++j)
                {
                    M[i][j] = volatilityData[i][j];
                }
            }
            return M;
        }
    }
}