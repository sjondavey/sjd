#include <sjd/rates/Forward/ForwardCurve.h>

#include <sjd/rates/Discount/DiscountCurves.h>
#include <sjd/rates/Forward/Deterministic/ForwardCurveCMADeterministic.h>
#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>
#include <sjd/rates/Forward/ForwardCurveFX.h>
#include <sjd/rates/Forward/UsesInterestRateDifferential.h>

#include <algorithm>
#include <cmath>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    ForwardCurve::ForwardCurve()
        : quoteStyle(ForwardQuote::Interpolated), averagingStyle(ForwardAveraging::Point),
          rollStyle(ForwardRoll::Deterministic), extrapolation(false), spot(0.0)
    {
    }

    void ForwardCurve::setInterpolated(Date anchorDate,
                                       vector<Date> observationDates,
                                       vector<double> forwards,
                                       ArrayInterpolatorType interpolation,
                                       bool allowExtrapolation,
                                       ForwardAveraging averaging,
                                       ForwardRoll roll,
                                       std::shared_ptr<VolatilityRateSource> volatility)
    {
        quoteStyle = ForwardQuote::Interpolated;
        averagingStyle = averaging;
        rollStyle = roll;
        vol = volatility;
        datedCurve.reset();
        monthCurve.reset();

        if (averaging == ForwardAveraging::CalendarMonth)
        {
            monthCurve = std::shared_ptr<CalendarMonthAverageCurve>(new CalendarMonthAverageCurve(
                anchorDate, observationDates, forwards, interpolation, allowExtrapolation));
            anchor = monthCurve->getAnchorDate();
            extrapolation = monthCurve->getAllowsExtrapolation();
            final = monthCurve->getFinalDate();
            string errorMessage;
            if (monthCurve->isOK(errorMessage) && !extrapolation && !monthCurve->containsRateOnAnchorDate())
            {
                monthCurve->addPointOnAnchorDate(forwards.front());
            }
            originalObservationDates = monthCurve->getObservationDates();
            originalObservationRates = monthCurve->getRates();
            return;
        }

        datedCurve = std::shared_ptr<DatedCurve>(
            new DatedCurve(anchorDate, observationDates, forwards, interpolation, allowExtrapolation));
        anchor = datedCurve->getAnchorDate();
        extrapolation = datedCurve->getAllowsExtrapolation();
        final = datedCurve->getFinalDate();
        string errorMessage;
        if (datedCurve->isOK(errorMessage) && !extrapolation && !datedCurve->containsRateOnAnchorDate())
        {
            datedCurve->addPointOnAnchorDate(forwards.front());
        }
    }

    void ForwardCurve::setInterestDifferential(Date anchorDate,
                                               double spotAtT0,
                                               std::shared_ptr<DiscountRateSource> domesticCurve,
                                               std::shared_ptr<DiscountRateSource> foreignCurve,
                                               ForwardRoll roll,
                                               std::shared_ptr<VolatilityRateSource> volatility)
    {
        quoteStyle = ForwardQuote::InterestDifferential;
        averagingStyle = ForwardAveraging::Point;
        rollStyle = roll;
        vol = volatility;
        datedCurve.reset();
        monthCurve.reset();
        setSpotAtT0(spotAtT0);
        setRateSources(domesticCurve, foreignCurve);
        anchor = anchorDate;
    }

    Date ForwardCurve::anchorDate() const
    {
        return anchor;
    }
    Date ForwardCurve::finalDate() const
    {
        return final;
    }
    bool ForwardCurve::allowsExtrapolation() const
    {
        return extrapolation;
    }
    ForwardAveraging ForwardCurve::averaging() const
    {
        return averagingStyle;
    }
    ForwardRoll ForwardCurve::roll() const
    {
        return rollStyle;
    }
    ForwardQuote ForwardCurve::quote() const
    {
        return quoteStyle;
    }

    bool ForwardCurve::quotesAreOk(string& errorMessage)
    {
        if (monthCurve)
        {
            return monthCurve->isOK(errorMessage);
        }
        if (datedCurve)
        {
            return datedCurve->isOK(errorMessage);
        }
        errorMessage.clear();
        return true;
    }

    double ForwardCurve::getForward(Date toDate) const
    {
        if (quoteStyle == ForwardQuote::InterestDifferential)
        {
            return spot * foreign->getDiscountFactor(toDate) / domestic->getDiscountFactor(toDate);
        }
        if (monthCurve)
        {
            return monthCurve->getRate(toDate);
        }
        return datedCurve->getRate(toDate);
    }

    vector<double> ForwardCurve::getForward(const vector<Date>& toDates) const
    {
        if (toDates.size() < 1)
        {
            return vector<double>(0);
        }
        if (monthCurve)
        {
            return monthCurve->getRate(toDates);
        }
        vector<double> forwards(toDates.size());
        for (size_t i = 0; i < toDates.size(); ++i)
        {
            forwards[i] = getForward(toDates[i]);
        }
        return forwards;
    }

    double ForwardCurve::getForward(Month month, Year year) const
    {
        return getForward(Date(1, month, year));
    }

    vector<Date> ForwardCurve::observationDates() const
    {
        if (monthCurve)
        {
            return monthCurve->getObservationDates();
        }
        if (datedCurve)
        {
            return datedCurve->getObservationDates();
        }
        return vector<Date>();
    }

    vector<double> ForwardCurve::rates() const
    {
        if (monthCurve)
        {
            return monthCurve->getRates();
        }
        return datedCurve->getRates();
    }

    vector<double> ForwardCurve::observationPoints() const
    {
        if (monthCurve)
        {
            return monthCurve->getObservationPoints();
        }
        return rates();
    }

    void ForwardCurve::setForwards(vector<double> updatedForwards)
    {
        if (monthCurve)
        {
            monthCurve->setRates(updatedForwards);
            return;
        }
        if (datedCurve)
        {
            datedCurve->setRates(updatedForwards);
        }
    }

    ArrayInterpolatorType ForwardCurve::interpolation() const
    {
        if (monthCurve)
        {
            return monthCurve->getType();
        }
        if (datedCurve)
        {
            return datedCurve->getType();
        }
        return LINEAR;
    }

    void ForwardCurve::setSpotAtT0(double spotAtT0)
    {
        spot = spotAtT0;
    }
    double ForwardCurve::spotAtT0() const
    {
        return spot;
    }

    void ForwardCurve::setRateSources(std::shared_ptr<DiscountRateSource> domesticCurve,
                                      std::shared_ptr<DiscountRateSource> foreignCurve)
    {
        domestic = domesticCurve;
        foreign = foreignCurve;
        extrapolation = domestic->getAllowsExtrapolation() && foreign->getAllowsExtrapolation();
        final = min(domestic->getFinalDate(), foreign->getFinalDate());
    }

    std::shared_ptr<DiscountRateSource> ForwardCurve::domesticRateSource() const
    {
        return domestic;
    }
    std::shared_ptr<DiscountRateSource> ForwardCurve::foreignRateSource() const
    {
        return foreign;
    }
    std::shared_ptr<VolatilityRateSource> ForwardCurve::volatility() const
    {
        return vol;
    }

    bool ForwardCurve::calendarMonthInRange(Date date) const
    {
        Date beginningOfMonth = Date(1, date.month(), date.year());
        Date beginningOfAnchorDate = Date(1, anchor.month(), anchor.year());
        if (extrapolation)
        {
            return beginningOfMonth >= beginningOfAnchorDate;
        }
        return beginningOfMonth >= beginningOfAnchorDate && beginningOfMonth <= final;
    }

    std::shared_ptr<ForwardRateSource> ForwardCurve::rolledInterpolated(
        Date toDate,
        const vector<Date>& dates,
        const vector<double>& forwards,
        ArrayInterpolatorType interpolationType,
        bool allowExtrapolation) const
    {
        if (averagingStyle == ForwardAveraging::CalendarMonth)
        {
            return std::shared_ptr<ForwardCurveCMADeterministic>(new ForwardCurveCMADeterministic(
                toDate, dates, forwards, interpolationType, allowExtrapolation));
        }
        return std::shared_ptr<ForwardCurveDeterministic>(
            new ForwardCurveDeterministic(toDate, dates, forwards, interpolationType, allowExtrapolation));
    }

    std::shared_ptr<ForwardRateSource> ForwardCurve::parallelBump(double spread)
    {
        if (quoteStyle == ForwardQuote::InterestDifferential)
        {
            return std::shared_ptr<ForwardCurveFX>(
                new ForwardCurveFX(anchor, spot + spread, domestic, foreign));
        }

        vector<Date> observationDates = this->observationDates();
        vector<double> bumpedForwards;
        for (size_t i = 0; i < observationDates.size(); ++i)
        {
            bumpedForwards.push_back(getForward(observationDates[i]) + spread);
        }
        if (averagingStyle == ForwardAveraging::CalendarMonth)
        {
            return rolledInterpolated(anchor, observationDates, bumpedForwards, LINEAR, true);
        }
        return rolledInterpolated(anchor, observationDates, bumpedForwards, interpolation(),
                                  allowsExtrapolation());
    }

    std::shared_ptr<ForwardRateSource> ForwardCurve::rollForward(Date toDate, bool inRange)
    {
        if (quoteStyle == ForwardQuote::InterestDifferential)
        {
            double rolledSpot = getForward(toDate);
            vector<Date> rolledDRSDates = rolledDatesAheadOf(toDate);
            vector<double> domesticDF(rolledDRSDates.size());
            vector<double> foreignDF(rolledDRSDates.size());
            double domesticScalingFactor = domestic->getDiscountFactor(toDate);
            double foreignScalingFactor = foreign->getDiscountFactor(toDate);
            for (size_t i = 0; i < rolledDRSDates.size(); ++i)
            {
                domesticDF[i] = domestic->getDiscountFactor(rolledDRSDates[i]) / domesticScalingFactor;
                foreignDF[i] = foreign->getDiscountFactor(rolledDRSDates[i]) / foreignScalingFactor;
            }
            std::shared_ptr<DiscountRateSource> rolledDomestic = std::shared_ptr<DiscountRateSource>(
                new DiscountCurve(toDate, rolledDRSDates, domesticDF, LINEAR, extrapolation));
            std::shared_ptr<DiscountRateSource> rolledForeign = std::shared_ptr<DiscountRateSource>(
                new DiscountCurve(toDate, rolledDRSDates, foreignDF, LINEAR, extrapolation));
            return std::shared_ptr<ForwardRateSource>(
                new ForwardCurveFX(toDate, rolledSpot, rolledDomestic, rolledForeign));
        }

        vector<Date> rolledObservationDates;
        vector<double> rolledForwards;
        vector<Date> observationDates = this->observationDates();
        if (!inRange)
        {
            rolledObservationDates.push_back(toDate);
            rolledForwards.push_back(1.0);
        }
        else
        {
            for (size_t i = 0; i < observationDates.size(); ++i)
            {
                if (observationDates[i] >= toDate)
                {
                    rolledObservationDates.push_back(observationDates[i]);
                    rolledForwards.push_back(getForward(observationDates[i]));
                }
            }
        }
        if (averagingStyle == ForwardAveraging::CalendarMonth)
        {
            return rolledInterpolated(toDate, rolledObservationDates, rolledForwards, LINEAR, true);
        }
        return rolledInterpolated(toDate, rolledObservationDates, rolledForwards, interpolation(),
                                  allowsExtrapolation());
    }

    std::shared_ptr<ForwardRateSource> ForwardCurve::rollForward(Date toDate, double normalRV, bool inRange)
    {
        if (quoteStyle == ForwardQuote::InterestDifferential)
        {
            double rolledSpot = getForward(toDate);
            double sd = vol->getStandardDeviation(toDate);
            rolledSpot *= exp(-sd * sd / 2 + sd * normalRV);
            vector<Date> rolledDRSDates = rolledDatesAheadOf(toDate);
            vector<double> domesticDF(rolledDRSDates.size());
            vector<double> foreignDF(rolledDRSDates.size());
            double domesticScalingFactor = domestic->getDiscountFactor(toDate);
            double foreignScalingFactor = foreign->getDiscountFactor(toDate);
            for (size_t i = 0; i < rolledDRSDates.size(); ++i)
            {
                domesticDF[i] = domestic->getDiscountFactor(rolledDRSDates[i]) / domesticScalingFactor;
                foreignDF[i] = foreign->getDiscountFactor(rolledDRSDates[i]) / foreignScalingFactor;
            }
            std::shared_ptr<DiscountRateSource> rolledDomestic = std::shared_ptr<DiscountRateSource>(
                new DiscountCurve(toDate, rolledDRSDates, domesticDF, LINEAR, extrapolation));
            std::shared_ptr<DiscountRateSource> rolledForeign = std::shared_ptr<DiscountRateSource>(
                new DiscountCurve(toDate, rolledDRSDates, foreignDF, LINEAR, extrapolation));
            return std::shared_ptr<ForwardRateSource>(
                new ForwardCurveFX(toDate, rolledSpot, rolledDomestic, rolledForeign));
        }

        vector<Date> rolledObservationDates;
        vector<double> rolledForwards;
        vector<Date> observationDates = this->observationDates();
        if (!inRange)
        {
            rolledObservationDates.push_back(toDate);
            rolledForwards.push_back(1.0);
        }
        else if (rollStyle == ForwardRoll::SpotVolatility)
        {
            double sd = vol->getStandardDeviation(toDate);
            double multiplicativeFactor = exp(-(sd * sd) / 2 + sd * normalRV);
            for (size_t i = 0; i < observationDates.size(); ++i)
            {
                if (observationDates[i] >= toDate)
                {
                    rolledObservationDates.push_back(observationDates[i]);
                    rolledForwards.push_back(getForward(observationDates[i]) * multiplicativeFactor);
                }
            }
        }
        else
        {
            double sqrtDt = sqrt((toDate - anchor) / 365.0);
            Date start = toDate;
            if (averagingStyle == ForwardAveraging::CalendarMonth)
            {
                start = Date(1, toDate.month(), toDate.year());
                observationDates =
                    originalObservationDates.empty() ? observationDates : originalObservationDates;
            }
            for (size_t i = 0; i < observationDates.size(); ++i)
            {
                if (observationDates[i] >= start)
                {
                    Date d1 = observationDates[i];
                    rolledObservationDates.push_back(d1);
                    double sd = vol->getVolatility(d1) * sqrtDt;
                    rolledForwards.push_back(getForward(d1) * exp(-sd * sd / 2 + sd * normalRV));
                }
            }
        }
        return rolledInterpolated(toDate, rolledObservationDates, rolledForwards, LINEAR, true);
    }

    vector<Date> ForwardCurve::rolledDatesAheadOf(Date date) const
    {
        vector<Date> datesToUse;
        const int numberOfSingleMonths = 6;
        const int numberOfQuarters = 4;
        Date tmpDate = date;
        for (int months = 0; tmpDate < final && months < numberOfSingleMonths; ++months)
        {
            datesToUse.push_back(tmpDate);
            tmpDate += 30;
        }
        for (int quarters = 0; tmpDate < final && quarters < numberOfQuarters; ++quarters)
        {
            datesToUse.push_back(tmpDate);
            tmpDate += 92;
        }
        while (tmpDate < final)
        {
            datesToUse.push_back(tmpDate);
            tmpDate += 365;
        }
        datesToUse.push_back(final);
        return datesToUse;
    }

    void ForwardCurve::preparePath(const vector<Date>& forwardSimulationDates,
                                   const vector<Date>& monthStarts)
    {
        pathCurves.clear();
        forwardDeviations.clear();
        spotDeviations.clear();
        forwardIndexes.clear();

        if (quoteStyle == ForwardQuote::InterestDifferential)
        {
            for (size_t i = 0; i < forwardSimulationDates.size(); ++i)
            {
                double rolledSpot = getForward(forwardSimulationDates[i]);
                vector<Date> rolledDRSDates = rolledDatesAheadOf(forwardSimulationDates[i]);
                vector<double> domesticDF(rolledDRSDates.size());
                vector<double> foreignDF(rolledDRSDates.size());
                for (size_t j = 0; j < rolledDRSDates.size(); ++j)
                {
                    domesticDF[j] = domestic->getDiscountFactor(rolledDRSDates[j]);
                    foreignDF[j] = foreign->getDiscountFactor(rolledDRSDates[j]);
                }
                std::shared_ptr<DiscountRateSource> rolledDomestic =
                    std::shared_ptr<DiscountRateSource>(new DiscountCurve(
                        forwardSimulationDates[i], rolledDRSDates, domesticDF, LINEAR, extrapolation));
                std::shared_ptr<DiscountRateSource> rolledForeign =
                    std::shared_ptr<DiscountRateSource>(new DiscountCurve(
                        forwardSimulationDates[i], rolledDRSDates, foreignDF, LINEAR, extrapolation));
                pathCurves.push_back(std::shared_ptr<ForwardCurveFX>(new ForwardCurveFX(
                    forwardSimulationDates[i], rolledSpot, rolledDomestic, rolledForeign)));
                spotDeviations.push_back(vol->getStandardDeviation(forwardSimulationDates[i]));
            }
            return;
        }

        forwardIndexes = vector<size_t>(forwardSimulationDates.size(), 0);
        vector<Date> sourceDates = observationDates();
        if (averagingStyle == ForwardAveraging::CalendarMonth && !originalObservationDates.empty())
        {
            sourceDates = originalObservationDates;
        }
        for (size_t i = 0; i < forwardSimulationDates.size(); ++i)
        {
            vector<Date> datesSubset;
            vector<double> ratesSubset;
            bool indexFound = false;
            Date start = forwardSimulationDates[i];
            if (averagingStyle == ForwardAveraging::CalendarMonth && i < monthStarts.size())
            {
                start = monthStarts[i];
            }
            for (size_t j = 0; j < sourceDates.size(); ++j)
            {
                if (sourceDates[j] >= start)
                {
                    if (!indexFound)
                    {
                        forwardIndexes[i] = j;
                        indexFound = true;
                    }
                    datesSubset.push_back(sourceDates[j]);
                    ratesSubset.push_back(getForward(sourceDates[j]));
                }
            }
            pathCurves.push_back(
                rolledInterpolated(forwardSimulationDates[i], datesSubset, ratesSubset, LINEAR, true));
        }

        if (rollStyle == ForwardRoll::SpotVolatility)
        {
            // rollPath compounds each step onto the previous one, so each entry is the
            // standard deviation of that step alone, not the total from the anchor.
            spotDeviations = vector<double>(forwardSimulationDates.size());
            double previousSd = 0.0;
            for (size_t i = 0; i < spotDeviations.size(); ++i)
            {
                double sd = vol->getStandardDeviation(forwardSimulationDates[i]);
                spotDeviations[i] = sqrt(std::max(0.0, sd * sd - previousSd * previousSd));
                previousSd = sd;
            }
            return;
        }

        vector<Date> rolledCurveDates;
        if (averagingStyle == ForwardAveraging::CalendarMonth)
        {
            rolledCurveDates =
                std::static_pointer_cast<ForwardCurveCMADeterministic>(pathCurves[0])->getObservationDates();
        }
        else
        {
            rolledCurveDates =
                std::static_pointer_cast<ForwardCurveDeterministic>(pathCurves[0])->getObservationDates();
        }
        vector<double> sds(rolledCurveDates.size(), 0);
        double sqrtdt = sqrt((forwardSimulationDates[0] - anchor) / 365.0);
        for (size_t j = 0; j < rolledCurveDates.size(); ++j)
        {
            sds[j] = vol->getVolatility(rolledCurveDates[j]) * sqrtdt;
        }
        forwardDeviations.push_back(sds);
        for (size_t i = 1; i < forwardSimulationDates.size(); ++i)
        {
            sqrtdt = sqrt((forwardSimulationDates[i] - forwardSimulationDates[i - 1]) / 365.0);
            if (averagingStyle == ForwardAveraging::CalendarMonth)
            {
                rolledCurveDates = std::static_pointer_cast<ForwardCurveCMADeterministic>(pathCurves[i])
                                       ->getObservationDates();
            }
            else
            {
                rolledCurveDates =
                    std::static_pointer_cast<ForwardCurveDeterministic>(pathCurves[i])->getObservationDates();
            }
            sds = vector<double>(rolledCurveDates.size(), 0);
            for (size_t j = 0; j < rolledCurveDates.size(); ++j)
            {
                sds[j] = vol->getVolatility(rolledCurveDates[j]) * sqrtdt;
            }
            forwardDeviations.push_back(sds);
        }
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurve::rollPath(
        const vector<double>& normalRVs, const vector<Date>& forwardSimulationDates)
    {
        vector<std::shared_ptr<ForwardRateSource>> rolled(pathCurves.size());
        if (quoteStyle == ForwardQuote::InterestDifferential)
        {
            for (size_t i = 0; i < forwardSimulationDates.size(); ++i)
            {
                double bumpedSpot = getForward(forwardSimulationDates[i]);
                bumpedSpot *=
                    exp(-spotDeviations[i] * spotDeviations[i] / 2 + spotDeviations[i] * normalRVs[i]);
                std::shared_ptr<UsesInterestRateDifferential> fx =
                    std::dynamic_pointer_cast<UsesInterestRateDifferential>(pathCurves[i]);
                fx->setSpotAtT0(bumpedSpot);
                rolled[i] = pathCurves[i];
            }
            return rolled;
        }

        vector<double> fwds = rates();
        for (size_t i = 0; i < forwardSimulationDates.size(); ++i)
        {
            size_t startIndex = forwardIndexes[i];
            if (rollStyle == ForwardRoll::SpotVolatility)
            {
                double sd = spotDeviations[i];
                for (size_t j = startIndex; j < fwds.size(); ++j)
                {
                    fwds[j] *= exp(-sd * sd / 2 + sd * normalRVs[i]);
                }
            }
            else
            {
                for (size_t j = startIndex; j < fwds.size(); ++j)
                {
                    double sd = forwardDeviations[i][j - startIndex];
                    fwds[j] *= exp(-sd * sd / 2 + sd * normalRVs[i]);
                }
            }
            if (averagingStyle == ForwardAveraging::CalendarMonth)
            {
                std::static_pointer_cast<ForwardCurveCMADeterministic>(pathCurves[i])
                    ->setForwards(vector<double>(fwds.begin() + forwardIndexes[i], fwds.end()));
            }
            else
            {
                std::static_pointer_cast<ForwardCurveDeterministic>(pathCurves[i])
                    ->setForwards(vector<double>(fwds.begin() + forwardIndexes[i], fwds.end()));
            }
            rolled[i] = pathCurves[i];
        }
        return rolled;
    }

    vector<double> ForwardCurve::impliedRandomNumbers(vector<double> spotPath,
                                                      vector<double>& normalRVs,
                                                      Date anchorDate,
                                                      const vector<Date>& forwardSimulationDates)
    {
        vector<double> expectedForwards(forwardSimulationDates.size());
        vector<double> saved = normalRVs;
        if (forwardSimulationDates.front() == anchorDate)
        {
            normalRVs[0] = 0;
        }
        else
        {
            expectedForwards[0] = getForward(forwardSimulationDates[0]);
            normalRVs[0] = log(spotPath[0] / expectedForwards[0]) / forwardDeviations[0][0] +
                           forwardDeviations[0][0] / 2.0;
        }
        for (size_t i = 1; i < forwardSimulationDates.size(); ++i)
        {
            rollPath(normalRVs, forwardSimulationDates);
            expectedForwards[i] = pathCurves[i - 1]->getForward(forwardSimulationDates[i]);
            normalRVs[i] = log(spotPath[i] / expectedForwards[i]) / forwardDeviations[i][0] +
                           forwardDeviations[i][0] / 2.0;
        }
        vector<double> randomNumbers = normalRVs;
        normalRVs = saved;
        return randomNumbers;
    }

}
