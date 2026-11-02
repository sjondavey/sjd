#include <sjd/rates/Volatility/VolatilitySurfaceMoneyness.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    VolatilitySurfaceMoneyness

     =======================================================================================*/
    VolatilitySurfaceMoneyness::VolatilitySurfaceMoneyness(Date anchorDateInput,
                                                           vector<Date> observationDatesInput,
                                                           vector<double> moneynessInput,
                                                           vector<vector<double>> volatilityDataInput,
                                                           SurfaceInterpolatorType interpolatorTypeInput,
                                                           bool extrapolateInput)
    {
        errorTracking =
            std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("VolatilitySurfaceMoneyness"));

        setInputs(anchorDateInput, observationDatesInput, moneynessInput,
                  convertVectorOfVectorsToMatrix(volatilityDataInput), interpolatorTypeInput,
                  extrapolateInput);
    }

    VolatilitySurfaceMoneyness::VolatilitySurfaceMoneyness(Date anchorDateInput,
                                                           vector<Date> observationDatesInput,
                                                           vector<double> moneynessInput,
                                                           QuantLib::Matrix volatilityDataInput,
                                                           SurfaceInterpolatorType interpolatorTypeInput,
                                                           bool extrapolateInput)
    {
        errorTracking =
            std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("VolatilitySurfaceMoneyness"));

        setInputs(anchorDateInput, observationDatesInput, moneynessInput, volatilityDataInput,
                  interpolatorTypeInput, extrapolateInput);
    }

    bool VolatilitySurfaceMoneyness::isOK()
    {
        VolatiltiySurfaceRateSource::isOK();
        return !errorTracking->getHasErrors();
    }

    bool VolatilitySurfaceMoneyness::isInRange(QuantLib::Date date) const
    {
        return timeInRange(dayCounter->yearFraction(anchorDate, date));
    }

    bool VolatilitySurfaceMoneyness::isInRange(QuantLib::Date date, double strike, double forward) const
    {
        return isInRange(date);
    }

    double VolatilitySurfaceMoneyness::getVolatility(Date toDate) const
    {
        if (!isInRange(toDate))
        {
            return numeric_limits<double>::quiet_NaN();
        }
        double yf = dayCounter->yearFraction(anchorDate, toDate);
        return surfaceVolatility(yf, 0.0);
    }

    double VolatilitySurfaceMoneyness::getVolatility(Date toDate, double strike, double forward) const
    {
        if (!isInRange(toDate, strike, forward))
        {
            return numeric_limits<double>::quiet_NaN();
        }
        double yf = dayCounter->yearFraction(anchorDate, toDate);
        double moneyness = (strike - forward) / forward;
        return surfaceVolatility(yf, moneyness);
    }

    std::shared_ptr<VolatilityRateSource> VolatilitySurfaceMoneyness::parallelBump(double spread)
    {
        vector<Date> newDates;
        QuantLib::Matrix volatilitySubset;

        getRolledVariables(anchorDate, newDates, volatilitySubset);
        Matrix spreadMatrix = Matrix(volatilitySubset.rows(), volatilitySubset.columns(), spread);
        Matrix bumpedVolMatrix = volatilitySubset + spreadMatrix;

        return std::shared_ptr<VolatilityRateSource>(new VolatilitySurfaceMoneyness(
            anchorDate, newDates, strikeDimension, bumpedVolMatrix, interpolatorType, allowExtrapolation));
    }

    std::shared_ptr<VolatilityRateSource> VolatilitySurfaceMoneyness::rollForward(Date toDate)
    {
        vector<Date> newDates;
        QuantLib::Matrix volatilitySubset;

        getRolledVariables(toDate, newDates, volatilitySubset);

        return std::shared_ptr<VolatilityRateSource>(new VolatilitySurfaceMoneyness(
            toDate, newDates, strikeDimension, volatilitySubset, interpolatorType, allowExtrapolation));
    }
}