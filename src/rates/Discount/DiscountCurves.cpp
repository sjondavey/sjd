#include <sjd/rates/Discount/DiscountCurves.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    /*======================================================================================
    DiscountCurve
    =======================================================================================*/
    DiscountCurve::DiscountCurve()
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("DiscountCurve"));
        resetErrorMessages();
    }

    DiscountCurve::DiscountCurve(Date anchorDateInput,
                                 vector<Date> observationDatesInput,
                                 vector<double> discountFactorsInput,
                                 ArrayInterpolatorType typeInput,
                                 bool allowExtrapolationInput)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("DiscountCurve"));

        datedCurve = std::shared_ptr<DatedCurve>(new DatedCurve(anchorDateInput, observationDatesInput,
                                                                discountFactorsInput, typeInput,
                                                                allowExtrapolationInput));
        anchorDate = datedCurve->getAnchorDate();
        allowExtrapolation = datedCurve->getAllowsExtrapolation();
        finalDate = datedCurve->getFinalDate();
        type = typeInput;
        string tmp;
        if (datedCurve->isOK(tmp) && (!datedCurve->containsRateOnAnchorDate()))
        {
            datedCurve->addPointOnAnchorDate(1.0);
        }
    }

    bool DiscountCurve::isOK()
    {
        DeterministicDiscountRateSource::isOK();
        string errorMessage = "";
        if (!datedCurve->isOK(errorMessage))
        {
            const std::string code = errorMessage.find("same dimension") != std::string::npos
                                         ? "DISCOUNT_CURVE_DIMENSION_MISMATCH"
                                         : "DISCOUNT_CURVE_INVALID";
            recordIssue(code, errorMessage, {"inputs"});
            return false;
        }
        for (size_t i = 0; i < datedCurve->getObservationDates().size(); ++i)
        {
            bool hasNegativeInputs = false;
            if ((!hasNegativeInputs) && (datedCurve->getRates()[i] <= 0))
            {
                recordIssue("DISCOUNT_CURVE_NEGATIVE_FACTOR", "has negative inputs", {"discount_factors"});
                hasNegativeInputs = true;
            }
        }
        return !errorTracking->getHasErrors();
    }

    double DiscountCurve::getDiscountFactor(Date toDate) const
    {
        return datedCurve->getRate(toDate);
    }

    vector<Date> DiscountCurve::getObservationDates() const
    {
        return datedCurve->getObservationDates();
    }

    ArrayInterpolatorType DiscountCurve::getType()
    {
        return type;
    }

    std::shared_ptr<DiscountRateSource> DiscountCurve::parallelBump(
        double spread, std::shared_ptr<InterestRateConvention> convention)
    {
        vector<Date> curveDates = getObservationDates();
        vector<double> bumpedDiscountFactors = vector<double>(curveDates.size());
        size_t startIndex = 0;
        if (curveDates[0] == anchorDate)
        {
            bumpedDiscountFactors[0] = 1.0;
            ++startIndex;
        }
        for (size_t i = startIndex; i < curveDates.size(); ++i)
        {
            double df = getDiscountFactor(curveDates[i]);
            double rate = convention->dfToRate(df, anchorDate, curveDates[i]);
            bumpedDiscountFactors[i] = convention->rateToDF(rate + spread, anchorDate, curveDates[i]);
        }

        return std::shared_ptr<DiscountCurve>(new DiscountCurve(anchorDate, curveDates, bumpedDiscountFactors,
                                                                datedCurve->getType(), allowExtrapolation));
    }

    std::shared_ptr<DiscountRateSource> DiscountCurve::rollForward(Date toDate)
    {
        vector<Date> rolledObservationDates;
        vector<double> rolledDiscountFactors;
        if (!isInRange(toDate))
        {
            rolledObservationDates.push_back(toDate);
            rolledDiscountFactors.push_back(1.0);
        }
        else if (toDate == anchorDate)
        {
            rolledObservationDates = getObservationDates();
            rolledDiscountFactors = vector<double>(datedCurve->getRates().size());
            for (size_t i = 0; i < rolledDiscountFactors.size(); ++i)
            {
                rolledDiscountFactors[i] = (datedCurve->getRates()[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < getObservationDates().size(); ++i)
            {
                double dfToDate = getDiscountFactor(toDate);
                if (getObservationDates()[i] >= toDate)
                {
                    rolledObservationDates.push_back(getObservationDates()[i]);
                    rolledDiscountFactors.push_back(getDiscountFactor(getObservationDates()[i]) / dfToDate);
                }
            }
        }
        return std::shared_ptr<DiscountCurve>(new DiscountCurve(toDate, rolledObservationDates,
                                                                rolledDiscountFactors, datedCurve->getType(),
                                                                allowExtrapolation));
    }

    vector<pair<string, std::shared_ptr<RateSource>>> DiscountCurve::getMarketRateStresses()
    {
        vector<pair<string, std::shared_ptr<RateSource>>> marketStress =
            DeterministicDiscountRateSource::getMarketRateStresses();

        std::shared_ptr<Calendar> calendar = std::shared_ptr<Calendar>(new NullCalendar());
        vector<Date> threeMonthDates;
        vector<double> threeMonthDF;
        Date dateToAdd = anchorDate;
        double df = 1.0;
        while (dateToAdd <= finalDate)
        {
            threeMonthDates.push_back(dateToAdd);
            df = getDiscountFactor(dateToAdd);
            threeMonthDF.push_back(df);
            dateToAdd = calendar->advance(dateToAdd, 3, Months);
        }
        if (threeMonthDates.back() < finalDate)
        {
            threeMonthDates.push_back(finalDate);
            threeMonthDF.push_back(getDiscountFactor(finalDate));
        }
        dateToAdd = anchorDate;

        std::shared_ptr<DiscountCurve> dc = std::shared_ptr<DiscountCurve>(
            new DiscountCurve(anchorDate, threeMonthDates, threeMonthDF, LINEAR, allowExtrapolation));

        marketStress.push_back(make_pair("ZC Stress base", dc));

        std::shared_ptr<DayCounter> act360 = std::shared_ptr<DayCounter>(new Actual360());
        InterestRateConvention irc(act360, Compounded, Annual);

        for (size_t i = 1; i < threeMonthDates.size(); ++i)
        {
            dateToAdd = calendar->advance(dateToAdd, 3, Months);
            if (dateToAdd == threeMonthDates[i])
            {
                double originalDF = threeMonthDF[i];
                double adjustedDF =
                    irc.rateToDF(irc.dfToRate(originalDF + 0.0001, anchorDate, threeMonthDates[i]),
                                 anchorDate, threeMonthDates[i]);
                threeMonthDF[i] = adjustedDF;

                stringstream ss;
                ss << "DF 1 bp ZC Stress at " << i * 3 << "m";
                string description = ss.str();
                dc = std::shared_ptr<DiscountCurve>(
                    new DiscountCurve(anchorDate, threeMonthDates, threeMonthDF, LINEAR, allowExtrapolation));

                marketStress.push_back(make_pair(description, dc));
                threeMonthDF[i] = originalDF;
            }
        }
        return marketStress;
    }
}