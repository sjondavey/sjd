#ifndef SJD_FORWARD_CURVE_INCLUDED
#define SJD_FORWARD_CURVE_INCLUDED

#include <sjd/rates/Discount/DiscountRateSource.h>
#include <sjd/rates/Forward/ForwardRateSource.h>
#include <sjd/rates/Volatility/VolatilityRateSource.h>
#include <sjd/tools/CalendarMonthAverageCurve.h>
#include <sjd/tools/DatedCurve.h>

namespace sjd
{

    // How observation dates are turned into a forward.
    enum class ForwardAveraging
    {
        Point,
        CalendarMonth
    };

    // How the curve is moved to a later anchor date.
    enum class ForwardRoll
    {
        Deterministic,
        SpotVolatility,
        ForwardVolatility
    };

    // Where the forward number comes from.
    enum class ForwardQuote
    {
        Interpolated,
        InterestDifferential
    };

    /**
     * One forward curve. Interpolation, calendar-month averaging, FX via an
     * interest-rate differential, and the roll (deterministic, spot volatility,
     * or forward volatility) are policies held here.
     *
     * The older class names are constructors that pick one combination. A new
     * combination does not need a new type.
     *
     * The RateSource flatten rule still applies: from the anchor through the
     * first observation, the forward is the first observation.
     */
    class ForwardCurve
    {
    public:
        ForwardCurve();

        void setInterpolated(QuantLib::Date anchorDate,
                             std::vector<QuantLib::Date> observationDates,
                             std::vector<double> forwards,
                             ArrayInterpolatorType interpolation,
                             bool allowExtrapolation,
                             ForwardAveraging averaging,
                             ForwardRoll roll,
                             boost::shared_ptr<VolatilityRateSource> volatility);

        void setInterestDifferential(QuantLib::Date anchorDate,
                                     double spotAtT0,
                                     boost::shared_ptr<DiscountRateSource> domestic,
                                     boost::shared_ptr<DiscountRateSource> foreign,
                                     ForwardRoll roll,
                                     boost::shared_ptr<VolatilityRateSource> volatility);

        QuantLib::Date anchorDate() const;
        QuantLib::Date finalDate() const;
        bool allowsExtrapolation() const;
        ForwardAveraging averaging() const;
        ForwardRoll roll() const;
        ForwardQuote quote() const;

        bool quotesAreOk(std::string& errorMessage);
        double getForward(QuantLib::Date toDate) const;
        std::vector<double> getForward(const std::vector<QuantLib::Date>& toDates) const;
        double getForward(QuantLib::Month month, QuantLib::Year year) const;
        std::vector<QuantLib::Date> observationDates() const;
        std::vector<double> rates() const;
        std::vector<double> observationPoints() const;
        void setForwards(std::vector<double> updatedForwards);
        ArrayInterpolatorType interpolation() const;

        void setSpotAtT0(double spotAtT0);
        double spotAtT0() const;
        void setRateSources(boost::shared_ptr<DiscountRateSource> domestic,
                            boost::shared_ptr<DiscountRateSource> foreign);
        boost::shared_ptr<DiscountRateSource> domesticRateSource() const;
        boost::shared_ptr<DiscountRateSource> foreignRateSource() const;
        boost::shared_ptr<VolatilityRateSource> volatility() const;

        bool calendarMonthInRange(QuantLib::Date date) const;

        boost::shared_ptr<ForwardRateSource> parallelBump(double spread);
        boost::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate, bool inRange);
        boost::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate, double normalRV, bool inRange);

        void preparePath(const std::vector<QuantLib::Date>& forwardSimulationDates,
                         const std::vector<QuantLib::Date>& monthStarts);
        std::vector<boost::shared_ptr<ForwardRateSource>> rollPath(const std::vector<double>& normalRVs,
                                                                   const std::vector<QuantLib::Date>& forwardSimulationDates);
        std::vector<double> impliedRandomNumbers(std::vector<double> spot,
                                                 std::vector<double>& normalRVs,
                                                 QuantLib::Date anchor,
                                                 const std::vector<QuantLib::Date>& forwardSimulationDates);

    private:
        std::vector<QuantLib::Date> rolledDatesAheadOf(QuantLib::Date date) const;
        boost::shared_ptr<ForwardRateSource> rolledInterpolated(QuantLib::Date toDate,
                                                                const std::vector<QuantLib::Date>& dates,
                                                                const std::vector<double>& forwards,
                                                                ArrayInterpolatorType interpolation,
                                                                bool allowExtrapolation) const;

        ForwardQuote quoteStyle;
        ForwardAveraging averagingStyle;
        ForwardRoll rollStyle;

        QuantLib::Date anchor;
        QuantLib::Date final;
        bool extrapolation;

        boost::shared_ptr<DatedCurve> datedCurve;
        boost::shared_ptr<CalendarMonthAverageCurve> monthCurve;

        double spot;
        boost::shared_ptr<DiscountRateSource> domestic;
        boost::shared_ptr<DiscountRateSource> foreign;
        boost::shared_ptr<VolatilityRateSource> vol;

        std::vector<QuantLib::Date> originalObservationDates;
        std::vector<double> originalObservationRates;
        std::vector<size_t> forwardIndexes;
        std::vector<std::vector<double>> forwardDeviations;
        std::vector<double> spotDeviations;
        std::vector<boost::shared_ptr<ForwardRateSource>> pathCurves;
    };

}

#endif
