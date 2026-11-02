#include <sjd/rates/Forward/ForwardCurveFactory.h>

#include <sjd/rates/Forward/Deterministic/ForwardCurveCMADeterministic.h>
#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveCMAStochasticFwdVol.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFwdVol.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticSpotVol.h>

#include <stdexcept>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    std::shared_ptr<ForwardRateSource> makeForwardCurve(Date anchorDate,
                                                        vector<Date> observationDates,
                                                        vector<double> forwards,
                                                        ArrayInterpolatorType interpolation,
                                                        bool allowExtrapolation,
                                                        ForwardAveraging averaging,
                                                        ForwardRoll roll,
                                                        std::shared_ptr<VolatilityRateSource> volatility)
    {
        if (averaging == ForwardAveraging::CalendarMonth)
        {
            switch (roll)
            {
            case ForwardRoll::Deterministic:
                return std::make_shared<ForwardCurveCMADeterministic>(anchorDate, observationDates, forwards,
                                                                      interpolation, allowExtrapolation);
            case ForwardRoll::ForwardVolatility:
                return std::make_shared<ForwardCurveCMAStochasticFwdVol>(
                    anchorDate, observationDates, forwards, interpolation, allowExtrapolation, volatility);
            case ForwardRoll::SpotVolatility:
                throw invalid_argument("A calendar-month forward curve cannot roll with spot volatility. "
                                       "Use the forward-volatility roll.");
            }
        }
        switch (roll)
        {
        case ForwardRoll::Deterministic:
            return std::make_shared<ForwardCurveDeterministic>(anchorDate, observationDates, forwards,
                                                               interpolation, allowExtrapolation);
        case ForwardRoll::ForwardVolatility:
            return std::make_shared<ForwardCurveStochasticFwdVol>(
                anchorDate, observationDates, forwards, interpolation, allowExtrapolation, volatility);
        case ForwardRoll::SpotVolatility:
            return std::make_shared<ForwardCurveStochasticSpotVol>(
                anchorDate, observationDates, forwards, interpolation, allowExtrapolation, volatility);
        }
        throw invalid_argument("Unknown forward roll.");
    }
}
