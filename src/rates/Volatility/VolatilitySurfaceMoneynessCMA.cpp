#include <sjd/rates/Volatility/VolatilitySurfaceMoneynessCMA.h>

#include <sjd/tools/Errors.h>

#include <cmath>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    VolatilitySurfaceMoneynessCMA

    =======================================================================================*/
    VolatilitySurfaceMoneynessCMA::VolatilitySurfaceMoneynessCMA(
        Date anchorDateInput,
        vector<Date> observationDatesInput,
        vector<double> moneynessInput,
        vector<vector<double>> volatilityData,
        SurfaceInterpolatorType interpolatorTypeInput,
        bool extrapolate)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("VolatilitySurfaceMoneynessCMA");

        if (volatilityData.size() == 0)
        {
            return;
        }
        else
        {
            Matrix M = convertVectorOfVectorsToMatrix(volatilityData);
            setInputs(anchorDateInput, observationDatesInput, moneynessInput, M, interpolatorTypeInput,
                      extrapolate);
        }
    }

    VolatilitySurfaceMoneynessCMA::VolatilitySurfaceMoneynessCMA(
        Date anchorDateInput,
        vector<Date> observationDatesInput,
        vector<double> moneynessInput,
        QuantLib::Matrix volatilityInput,
        SurfaceInterpolatorType interpolatorTypeInput,
        bool extrapolate)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("VolatilitySurfaceMoneynessCMA");

        setInputs(anchorDateInput, observationDatesInput, moneynessInput, volatilityInput,
                  interpolatorTypeInput, extrapolate);
    }

    void VolatilitySurfaceMoneynessCMA::setInputs(Date anchorDateInput,
                                                  vector<Date> observationDatesInput,
                                                  vector<double> moneynessInput,
                                                  QuantLib::Matrix volatilityInput,
                                                  SurfaceInterpolatorType interpolatorTypeInput,
                                                  bool extrapolateInput)
    {
        if (observationDatesInput.size() == 0 || moneynessInput.size() == 0 ||
            volatilityInput.rows() * volatilityInput.columns() == 0)
        {
            observationDates = vector<Date>();
            strikeDimension = vector<double>();
            volatility = Matrix(0, 0);
            finalDate = Date(01, Jan, 1901);
            return;
        }

        allowExtrapolation = extrapolateInput;
        anchorDate = anchorDateInput;
        dayCounter = std::make_shared<Actual365Fixed>();
        simpleDayCounter = std::make_shared<SimpleDayCounter>();

        strikeDimension = moneynessInput;
        observationDates = observationDatesInput;
        beginningOfAnchorDateMonth = Date(1, anchorDate.month(), anchorDate.year());
        Date beginningOfMonth = Date(1, observationDates[0].month(), observationDates[0].year());
        if (beginningOfMonth > beginningOfAnchorDateMonth)
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
            Date monthStart = Date(1, observationDates[i].month(), observationDates[i].year());
            // observationDates.push_back(beginningOfMonth);
            double yf = simpleDayCounter->yearFraction(beginningOfAnchorDateMonth, monthStart);
            int monthsAsInt = static_cast<int>(std::lround(yf * 12));
            time.push_back((double)monthsAsInt); // Note that if there is more than one observation date per
                                                 // month the time vector will not be strictly increasing
        }
        finalDate = Date(1, observationDates.back().month(), observationDates.back().year());
        setInterpolatorType(interpolatorTypeInput);
    }

    void VolatilitySurfaceMoneynessCMA::runChecks()
    {
        VolatiltiySurfaceRateSource::runChecks();
        return;
    }

    double VolatilitySurfaceMoneynessCMA::getMonthsAheadOfAnchorDate(Date toDate) const
    {
        double t1 = dayCounter->yearFraction(beginningOfAnchorDateMonth, toDate);
        [[maybe_unused]] int t2 = static_cast<int>(std::lround(12 * t1));
        return (double)std::lround(12 * simpleDayCounter->yearFraction(beginningOfAnchorDateMonth, toDate));
    }

    bool VolatilitySurfaceMoneynessCMA::isInRange(QuantLib::Date date) const
    {
        Date beginningOfMonth = Date(1, date.month(), date.year());
        if (allowExtrapolation)
        {
            return (beginningOfMonth >= beginningOfAnchorDateMonth);
        }
        return ((beginningOfMonth >= beginningOfAnchorDateMonth) && (beginningOfMonth <= finalDate));
    };

    bool VolatilitySurfaceMoneynessCMA::isInRange(QuantLib::Date date,
                                                  double /*strike*/,
                                                  double /*forward*/) const
    {
        return isInRange(date);
    };

    bool VolatilitySurfaceMoneynessCMA::extendsTo(Date date) const
    {
        Date beginningOfMonth = Date(1, date.month(), date.year());
        return RateSource::extendsTo(beginningOfMonth);
    }

    double VolatilitySurfaceMoneynessCMA::getVolatility(Date toDate) const
    {
        SJD_ASSERT(isInRange(toDate));

        Date beginningOfMonth = Date(1, toDate.month(), toDate.year());
        double yf = simpleDayCounter->yearFraction(beginningOfAnchorDateMonth, beginningOfMonth);
        int monthsAsInt = static_cast<int>(std::lround(yf * 12)); // need to keep whole number of months ahead
        double monthAsDouble = ((double)monthsAsInt);

        return surfaceVolatility(monthAsDouble, 0.0);
    };

    double VolatilitySurfaceMoneynessCMA::getVolatility(Date toDate, double strike, double forward) const
    {
        SJD_ASSERT(isInRange(toDate, strike, forward));

        Date beginningOfMonth = Date(1, toDate.month(), toDate.year());
        double yf = simpleDayCounter->yearFraction(beginningOfAnchorDateMonth, beginningOfMonth);
        int monthsAsInt = static_cast<int>(std::lround(yf * 12)); // need to keep whole number of months ahead
        double monthAsDouble = ((double)monthsAsInt);

        double moneyness = (strike - forward) / forward;

        return surfaceVolatility(monthAsDouble, moneyness);
    }

    double VolatilitySurfaceMoneynessCMA::getStandardDeviation(Date toDate) const
    {
        double vol = getVolatility(toDate);
        double actualDt = dayCounter->yearFraction(anchorDate, toDate);
        return vol * sqrt(actualDt);
    };

    double VolatilitySurfaceMoneynessCMA::getStandardDeviation(Date toDate,
                                                               double strike,
                                                               double forward) const
    {
        double vol = getVolatility(toDate, strike, forward);
        double actualDt = dayCounter->yearFraction(anchorDate, toDate);
        return vol * sqrt(actualDt);
    };

    void VolatilitySurfaceMoneynessCMA::getVolAndSD(
        Date toDate, double strike, double forward, double& vol, double& sd) const
    {
        vol = getVolatility(toDate, strike, forward);
        double actualDt = dayCounter->yearFraction(anchorDate, toDate);
        sd = vol * sqrt(actualDt);
    };

    void VolatilitySurfaceMoneynessCMA::getVolAndSD(vector<Date> toDates,
                                                    double strike,
                                                    vector<double> forwards,
                                                    vector<double>& vols,
                                                    vector<double>& sds) const
    {
        if (toDates.size() == 0)
        {
            vols = vector<double>(0);
            sds = vector<double>(0);
            return;
        }
        size_t counter = 1;
        Date refDate = toDates[0];
        getVolAndSD(refDate, strike, forwards[0], vols[0], sds[0]);
        while (counter < toDates.size())
        {
            if (refDate.month() == toDates[counter].month() && refDate.year() == toDates[counter].year())
            {
                vols[counter] = vols[counter - 1];
                double actualDt = dayCounter->yearFraction(anchorDate, toDates[counter]);
                sds[counter] = vols[counter] * sqrt(actualDt);
            }
            else
            {
                refDate = toDates[counter];
                getVolAndSD(refDate, strike, forwards[counter], vols[counter], sds[counter]);
            }
            ++counter;
        }
    }

    std::shared_ptr<VolatilityRateSource> VolatilitySurfaceMoneynessCMA::parallelBump(double spread)
    {
        vector<Date> newDates;
        QuantLib::Matrix volatilitySubset;

        getRolledVariables(anchorDate, newDates, volatilitySubset);
        Matrix spreadMatrix = Matrix(volatilitySubset.rows(), volatilitySubset.columns(), spread);
        Matrix bumpedVolMatrix = volatilitySubset + spreadMatrix;

        return std::make_shared<VolatilitySurfaceMoneynessCMA>(
            anchorDate, newDates, strikeDimension, bumpedVolMatrix, interpolatorType, allowExtrapolation);
    }

    std::shared_ptr<VolatilityRateSource> VolatilitySurfaceMoneynessCMA::rollForward(Date toDate)
    {
        vector<Date> newDates;
        QuantLib::Matrix volatilitySubset;

        getRolledVariables(toDate, newDates, volatilitySubset);

        SurfaceInterpolatorType updatedType = interpolatorType;
        // if (toDate > anchorDate)
        //{
        //     updatedType = BILINEAR;
        // }

        return std::make_shared<VolatilitySurfaceMoneynessCMA>(
            toDate, newDates, strikeDimension, volatilitySubset, updatedType, allowExtrapolation);
    }

    void VolatilitySurfaceMoneynessCMA::getRolledVariables(Date toDate,
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
            Date beginningOfRollToDateMonth = Date(1, toDate.month(), toDate.year());
            size_t offset = 0;
            for (size_t i = 0; i < observationDates.size(); ++i)
            {
                double t1 = simpleDayCounter->yearFraction(beginningOfRollToDateMonth, observationDates[i]);
                [[maybe_unused]] int t2 = static_cast<int>(std::lround(12 * t1));
                double monthsFromAnchor = (double)std::lround(
                    12 * simpleDayCounter->yearFraction(beginningOfAnchorDateMonth, toDate));
                if (monthsFromAnchor >= 0)
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
}