#include "GenerateRatesEnvironments.h"

#include <boost/assign/std/vector.hpp>

using namespace std;
using namespace QuantLib;
using namespace boost::assign; // used to initialize vector

using namespace sjd;

GenerateRates::GenerateRates()
{
    defaultDayCounter = std::shared_ptr<DayCounter>(new Actual365Fixed());

    defaultCalendar = std::shared_ptr<Calendar>(new SouthAfrica());

    defaultAnchorDate = Date(27, Jan, 2012);

    defaultYears = 10;

    defaultCurveDates = boost::assign::list_of(defaultCalendar->advance(defaultAnchorDate, 1, Years))(
        defaultCalendar->advance(defaultAnchorDate, 2, Years))(defaultCalendar->advance(
        defaultAnchorDate, 3, Years))(defaultCalendar->advance(defaultAnchorDate, 4, Years))(
        defaultCalendar->advance(defaultAnchorDate, 5, Years))(defaultCalendar->advance(
        defaultAnchorDate, 6, Years))(defaultCalendar->advance(defaultAnchorDate, 7, Years))(
        defaultCalendar->advance(defaultAnchorDate, 8, Years))(
        defaultCalendar->advance(defaultAnchorDate, 9, Years));

    defaultSpot = 101.23;
    defaultNACCRate = 0.065;
    defaultNACCIRDiff = 0.045;
    defaultVolatiltiy = 0.2105;

    defaultSurfaceDates = boost::assign::list_of(defaultAnchorDate)(defaultCalendar->advance(
        defaultAnchorDate, 1, Months))(defaultCalendar->advance(defaultAnchorDate, 2, Months))(
        defaultCalendar->advance(defaultAnchorDate, 3, Months))(defaultCalendar->advance(
        defaultAnchorDate, 6, Months))(defaultCalendar->advance(defaultAnchorDate, 12, Months))(
        defaultCalendar->advance(defaultAnchorDate, 5, Years));

    defaultMoneyness += -1, -.5, 0, .5, 1;

    vector<double> v1, v2, v3, v4, v5;
    v1 += .17938, .182884, .193908, .219688, .248396, .263268, .263268;
    v2 += .17575, .17575, .18247, .206225, .234775, .2475, .2475;
    v3 += .175, .175, .18, .205, .235, .2475, .2475;
    v4 += .18825, .18825, .19547, .223725, .223725, .2725, .2725;
    v5 += .20128, .204784, .216708, .250288, .287796, .307068, .307068;
    defaultVolatilitySurface += v1, v2, v3, v4, v5;
}

std::shared_ptr<DiscountCurve> GenerateRates::getDiscountCurve(std::shared_ptr<DayCounter> dayCounter,
                                                               std::shared_ptr<Calendar> calendar,
                                                               Date anchorDate,
                                                               size_t years,
                                                               double flatNACCRate,
                                                               vector<Date> manditoryDates)
{

    vector<Date> dates = getDatesVector(calendar, anchorDate, years, manditoryDates);
    vector<double> discountFactors;
    for (size_t i = 0; i < dates.size(); ++i)
    {
        double yf = dayCounter->yearFraction(anchorDate, dates[i]);
        discountFactors.push_back(exp(-flatNACCRate * yf));
    }
    std::shared_ptr<DiscountCurve> dc =
        std::shared_ptr<DiscountCurve>(new DiscountCurve(anchorDate, dates, discountFactors, LINEAR, false));
    return dc;
}

std::shared_ptr<DiscountCurve> GenerateRates::getDefaultDiscountCurve(Date anchorDate)
{
    return getDiscountCurve(defaultDayCounter, defaultCalendar, anchorDate, defaultYears, defaultNACCRate,
                            defaultCurveDates);
}

std::shared_ptr<VolatilityCurve> GenerateRates::getVolatilityCurve(std::shared_ptr<DayCounter> dayCounter,
                                                                   std::shared_ptr<Calendar> calendar,
                                                                   Date anchorDate,
                                                                   size_t years,
                                                                   double flatNACCVolaitlity,
                                                                   vector<Date> manditoryDates)
{
    vector<Date> dates = getDatesVector(calendar, anchorDate, years, manditoryDates);
    vector<double> vol;
    for (size_t i = 0; i < dates.size(); ++i)
    {
        // double yf = dayCounter->yearFraction(anchorDate, dates[i]);
        vol.push_back(flatNACCVolaitlity);
    }
    std::shared_ptr<VolatilityCurve> vc =
        std::shared_ptr<VolatilityCurve>(new VolatilityCurve(anchorDate, dates, vol, dayCounter, LINEAR));
    return vc;
}

std::shared_ptr<VolatilityCurve> GenerateRates::getDefaultVolatiltiyCurve(Date anchorDate)
{
    return getVolatilityCurve(defaultDayCounter, defaultCalendar, anchorDate, defaultYears, defaultVolatiltiy,
                              defaultCurveDates);
}

std::shared_ptr<RatesEnvironment> GenerateRates::getRatesEnvironmentForCashflow(Date anchorDate,
                                                                                vector<Date> manditoryDates)
{
    std::shared_ptr<RatesEnvironment> re =
        std::shared_ptr<RatesEnvironment>(new RatesEnvironment(anchorDate));
    std::shared_ptr<DiscountCurve> dc = GenerateRates::getDiscountCurve(
        defaultDayCounter, defaultCalendar, anchorDate, defaultYears, defaultNACCRate, manditoryDates);
    re->setDRS(dc);
    return re;
}

std::shared_ptr<RatesEnvironment> GenerateRates::getRatesEnvironmentForEuropeanForward(
    Date anchorDate, double spot, double naccRFR, double naccIRDiff, vector<Date> manditoryDates)
{
    std::shared_ptr<RatesEnvironment> re =
        std::shared_ptr<RatesEnvironment>(new RatesEnvironment(anchorDate));
    std::shared_ptr<DiscountCurve> dc = GenerateRates::getDiscountCurve(
        defaultDayCounter, defaultCalendar, anchorDate, defaultYears, naccRFR, manditoryDates);
    std::shared_ptr<ForwardCurveDeterministic> fc = GenerateRates::getSimpleForwardCurve(
        defaultDayCounter, defaultCalendar, anchorDate, defaultYears, spot, naccIRDiff, manditoryDates);
    re->setDRS(dc);
    re->setFRS(fc);
    return re;
}

std::shared_ptr<RatesEnvironment> GenerateRates::getRatesEnvironmentForEuropeanForward(
    Date anchorDate, vector<Date> manditoryDates)
{
    std::shared_ptr<RatesEnvironment> re = getRatesEnvironmentForCashflow(anchorDate, manditoryDates);
    std::shared_ptr<ForwardCurveDeterministic> fc =
        GenerateRates::getSimpleForwardCurve(defaultDayCounter, defaultCalendar, anchorDate, defaultYears,
                                             defaultSpot, defaultNACCIRDiff, manditoryDates);
    re->setFRS(fc);
    return re;
}

std::shared_ptr<RatesEnvironment> GenerateRates::getRatesEnvironmentForAsianForward(
    Date anchorDate, vector<Date> manditoryDates)
{
    std::shared_ptr<RatesEnvironment> re = getRatesEnvironmentForCashflow(anchorDate, manditoryDates);

    std::shared_ptr<ForwardCurveCMAStochasticFwdVol> fc =
        getCalendarMonthAverageCurve(defaultDayCounter, defaultCalendar, anchorDate, defaultYears,
                                     defaultSpot, defaultNACCIRDiff, manditoryDates);
    re->setFRS(fc);

    vector<Date> historicFixingDates;
    historicFixingDates.push_back(defaultCalendar->advance(anchorDate, -1, Months));
    std::shared_ptr<DailyFixingCurve> fixingCurve =
        GenerateRates::buildFixingCurve(90.0, historicFixingDates);
    re->setFixingRS(fixingCurve);
    return re;
}

std::shared_ptr<RatesEnvironment> GenerateRates::getRatesEnvironmentForEuropeanOption(
    Date anchorDate, vector<Date> manditoryDates)
{
    std::shared_ptr<RatesEnvironment> re = getRatesEnvironmentForEuropeanForward(anchorDate, manditoryDates);

    std::shared_ptr<VolatilityCurve> vc = GenerateRates::getVolatilityCurve(
        defaultDayCounter, defaultCalendar, anchorDate, defaultYears, defaultVolatiltiy, manditoryDates);
    re->setVRS(vc);
    return re;
}

std::shared_ptr<RatesEnvironment> GenerateRates::getRatesEnvironmentForAsianOption(
    Date anchorDate, vector<Date> manditoryDates)
{
    std::shared_ptr<RatesEnvironment> re = getRatesEnvironmentForAsianForward(anchorDate, manditoryDates);

    std::shared_ptr<VolatilityRateSource> vs = getDefaultCalendarMonthAverageMoneynessSurface(anchorDate);
    re->setVRS(vs);
    return re;
}

std::shared_ptr<ForwardCurveDeterministic> GenerateRates::getSimpleForwardCurve(
    std::shared_ptr<DayCounter> dayCounter,
    std::shared_ptr<Calendar> calendar,
    Date anchorDate,
    size_t years,
    double spot,
    double flatNACCRateDifferential,
    vector<Date> manditoryDates)
{
    vector<Date> dates = getDatesVector(calendar, anchorDate, years, manditoryDates);
    vector<double> forwards;
    for (size_t i = 0; i < dates.size(); ++i)
    {
        double yf = dayCounter->yearFraction(anchorDate, dates[i]);
        forwards.push_back(spot * exp(flatNACCRateDifferential * yf));
    }
    std::shared_ptr<ForwardCurveDeterministic> fc = std::shared_ptr<ForwardCurveDeterministic>(
        new ForwardCurveDeterministic(anchorDate, dates, forwards));
    return fc;
}

std::shared_ptr<ForwardCurveCMAStochasticFwdVol> GenerateRates::getCalendarMonthAverageCurve(
    std::shared_ptr<DayCounter> dayCounter,
    std::shared_ptr<Calendar> calendar,
    Date anchorDate,
    size_t years,
    double spot,
    double flatNACCRateDifferential,
    vector<Date> manditoryDates)
{
    vector<Date> dates = getDatesVector(calendar, anchorDate, years, manditoryDates);
    vector<double> forwards;
    for (size_t i = 0; i < dates.size(); ++i)
    {
        double yf = dayCounter->yearFraction(anchorDate, dates[i]);
        forwards.push_back(spot * exp(flatNACCRateDifferential * yf));
    }
    std::shared_ptr<VolatilitySurfaceMoneynessCMA> vc =
        getDefaultCalendarMonthAverageMoneynessSurface(anchorDate);
    std::shared_ptr<ForwardCurveCMAStochasticFwdVol> fc = std::shared_ptr<ForwardCurveCMAStochasticFwdVol>(
        new ForwardCurveCMAStochasticFwdVol(anchorDate, dates, forwards, LINEAR, true, vc));
    return fc;
}

std::shared_ptr<VolatilitySurfaceMoneyness> GenerateRates::getDefaultMoneynessSurface(Date anchorDate)
{
    SurfaceInterpolatorType interpolatorType = BICUBIC;
    bool extrapolate = true;

    return std::shared_ptr<VolatilitySurfaceMoneyness>(
        new VolatilitySurfaceMoneyness(defaultAnchorDate, defaultSurfaceDates, defaultMoneyness,
                                       defaultVolatilitySurface, interpolatorType, extrapolate));
}

std::shared_ptr<VolatilitySurfaceMoneynessCMA> GenerateRates::getDefaultCalendarMonthAverageMoneynessSurface(
    Date anchorDate)
{
    vector<Date> observationDates;
    vector<double> moneyness;
    SurfaceInterpolatorType interpolatorType = BICUBIC;
    bool extrapolate = true;
    Calendar calendar = NullCalendar();
    observationDates.push_back(anchorDate);
    observationDates.push_back(calendar.advance(anchorDate, 1, Months));
    observationDates.push_back(calendar.advance(anchorDate, 2, Months));
    observationDates.push_back(calendar.advance(anchorDate, 3, Months));
    observationDates.push_back(calendar.advance(anchorDate, 6, Months));
    observationDates.push_back(calendar.advance(anchorDate, 10, Years));

    moneyness += -1, -.5, 0, .5, 1;

    vector<double> v1, v2, v3, v4, v5;
    v1 += .17938, .182884, .193908, .219688, .248396, .263268;
    v2 += .17575, .17575, .18247, .206225, .234775, .2475;
    v3 += .175, .175, .18, .205, .235, .2475;
    v4 += .18825, .18825, .19547, .223725, .223725, .2725;
    v5 += .20128, .204784, .216708, .250288, .287796, .307068;

    vector<vector<double>> volatility;
    volatility += v1, v2, v3, v4, v5;

    std::shared_ptr<DayCounter> dayCounter = std::shared_ptr<DayCounter>(new Actual365Fixed());
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    std::shared_ptr<ForwardRateSource> frs = GenerateRates::getSimpleForwardCurve(
        dayCounter, nullCalendar, anchorDate, 10, 100, .01, vector<Date>(0));

    return std::shared_ptr<VolatilitySurfaceMoneynessCMA>(new VolatilitySurfaceMoneynessCMA(
        anchorDate, observationDates, moneyness, volatility, interpolatorType, extrapolate));
}

std::shared_ptr<DailyFixingCurve> GenerateRates::buildFixingCurve(
    std::shared_ptr<ForwardCurveDeterministic> forwardCurve, vector<Date> fixingDates)
{
    vector<double> fixingRates;
    for (size_t i = 0; i < fixingDates.size(); ++i)
    {
        fixingRates.push_back(forwardCurve->getForward(fixingDates[i]));
    }

    std::shared_ptr<DailyFixingCurve> fixingCurve =
        std::shared_ptr<DailyFixingCurve>(new DailyFixingCurve(fixingDates, fixingRates));
    return fixingCurve;
}

std::shared_ptr<DailyFixingCurve> GenerateRates::buildFixingCurve(double fixedRate, vector<Date> fixingDates)
{
    vector<double> fixingRates;
    for (size_t i = 0; i < fixingDates.size(); ++i)
    {
        fixingRates.push_back(fixedRate);
    }

    std::shared_ptr<DailyFixingCurve> fixingCurve =
        std::shared_ptr<DailyFixingCurve>(new DailyFixingCurve(fixingDates, fixingRates));
    return fixingCurve;
}

vector<Date> GenerateRates::getDatesVector(std::shared_ptr<Calendar> calendar,
                                           Date anchorDate,
                                           size_t years,
                                           vector<Date> manditoryDates)
{
    set<Date> observationDates; // removes duplicates and keeps the order
    observationDates.insert(anchorDate);
    for (size_t i = 1; i <= years; ++i)
    {
        observationDates.insert(calendar->advance(anchorDate, i, Years));
    }
    if (!manditoryDates.empty())
    {
        for (size_t i = 0; i < manditoryDates.size(); ++i)
        {
            if (manditoryDates[i] > anchorDate)
            {
                observationDates.insert(manditoryDates[i]);
            }
        }
    }
    vector<Date> dates(observationDates.begin(), observationDates.end());
    return dates;
}
