#pragma once

#include <sjd/rates/Forward/ForwardCurve.h>
#include <sjd/rates/Forward/ForwardRateSource.h>
#include <sjd/rates/Volatility/VolatilityRateSource.h>

namespace sjd
{
    /**
     * Builds the interpolated forward curve for one averaging and roll combination.
     *
     * A Deterministic roll ignores volatility. A stochastic roll needs it; when it is
     * missing the curve still builds, and validation reports
     * FORWARD_VOLATILITY_RATE_SOURCE_MISSING.
     *
     * Calendar-month averaging has no spot-volatility roll. That combination throws
     * ConfigurationError.
     */
    std::shared_ptr<ForwardRateSource> makeForwardCurve(QuantLib::Date anchorDate,
                                                        std::vector<QuantLib::Date> observationDates,
                                                        std::vector<double> forwards,
                                                        ArrayInterpolatorType interpolation,
                                                        bool allowExtrapolation,
                                                        ForwardAveraging averaging,
                                                        ForwardRoll roll,
                                                        std::shared_ptr<VolatilityRateSource> volatility);
}
