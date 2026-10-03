#ifndef generateratesenvironments_test
#define generateratesenvironments_test

#include <iostream>

#include <sjd/rates/RatesEnvironment.h>

#include <sjd/rates/Discount/DiscountCurves.h>
#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministic.h>
#include <sjd/rates/Forward/Stochastic/ForwardCurveCMAStochasticFwdVol.h>
#include <sjd/rates/Volatility/VolatilityCurve.h>
#include <sjd/rates/Volatility/VolatilitySurfaces.h>
#include <sjd/rates/Volatility/VolatilitySurfaceMoneyness.h>
#include <sjd/rates/Volatility/VolatilitySurfaceMoneynessCMA.h>
#include <sjd/rates/Volatility/VolatilitySurfaceDelta.h>

#include <sjd/rates/Fixing/FixingCurves.h>

#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/all.hpp>
#include <ql/time/calendar.hpp>
#include <ql/time/calendars/southafrica.hpp>


#include <boost/assign/list_of.hpp> // To create static vectors with values

using namespace sjd;

/*======================================================================================
GenerateRates
    
Factory functions that generate generic rates sources and environments intended to make 
the testing of objects that require rate sources easier.
=======================================================================================*/
class GenerateRates 
{
  public:

    GenerateRates();

    std::shared_ptr<DiscountCurve> getDiscountCurve(std::shared_ptr<QuantLib::DayCounter> dayCounter,
                                                      std::shared_ptr<QuantLib::Calendar> calendar,
                                                      QuantLib::Date anchorDate, 
                                                      size_t years, 
                                                      double flatNACCRate, 
                                                      std::vector<QuantLib::Date> manditoryDates = std::vector<QuantLib::Date>());

    std::shared_ptr<DiscountCurve> getDefaultDiscountCurve(QuantLib::Date anchorDate);


    std::shared_ptr<ForwardCurveDeterministic> getSimpleForwardCurve(std::shared_ptr<QuantLib::DayCounter> dayCounter,
                                                                       std::shared_ptr<QuantLib::Calendar> calendar,
                                                                       QuantLib::Date anchorDate, 
                                                                       size_t years, 
                                                                       double spot, 
                                                                       double flatNACCRateDifferential, 
                                                                       std::vector<QuantLib::Date> manditoryDates = std::vector<QuantLib::Date>());

    std::shared_ptr<ForwardCurveCMAStochasticFwdVol> getCalendarMonthAverageCurve(std::shared_ptr<QuantLib::DayCounter> dayCounter,
                                                                                    std::shared_ptr<QuantLib::Calendar> calendar,
                                                                                    QuantLib::Date anchorDate, 
                                                                                    size_t years, 
                                                                                    double spot, 
                                                                                    double flatNACCRateDifferential, 
                                                                                    std::vector<QuantLib::Date> manditoryDates = std::vector<QuantLib::Date>());

    std::shared_ptr<VolatilityCurve> getVolatilityCurve(std::shared_ptr<QuantLib::DayCounter> dayCounter,
                                                          std::shared_ptr<QuantLib::Calendar> calendar,
                                                          QuantLib::Date anchorDate, 
                                                          size_t years, 
                                                          double flatNACCVolaitlity, 
                                                          std::vector<QuantLib::Date> manditoryDates = std::vector<QuantLib::Date>());

    std::shared_ptr<VolatilityCurve> getDefaultVolatiltiyCurve(QuantLib::Date anchorDate);
    std::shared_ptr<VolatilitySurfaceMoneyness> getDefaultMoneynessSurface(QuantLib::Date anchorDate);
    std::shared_ptr<VolatilitySurfaceMoneynessCMA> getDefaultCalendarMonthAverageMoneynessSurface(QuantLib::Date anchorDate);


    // Rates Environements
    std::shared_ptr<RatesEnvironment> getRatesEnvironmentForCashflow(QuantLib::Date anchorDate, 
                                                                       std::vector<QuantLib::Date> manditoryDates);

    std::shared_ptr<RatesEnvironment> getRatesEnvironmentForEuropeanForward(QuantLib::Date anchorDate,
                                                                              double spot, double 
                                                                              naccRFR, 
                                                                              double naccIRDiff,
                                                                              std::vector<QuantLib::Date> manditoryDates);

    std::shared_ptr<RatesEnvironment> getRatesEnvironmentForEuropeanForward(QuantLib::Date anchorDate,
                                                                              std::vector<QuantLib::Date> manditoryDates);

    std::shared_ptr<RatesEnvironment> getRatesEnvironmentForAsianForward(QuantLib::Date anchorDate,
                                                                           std::vector<QuantLib::Date> manditoryDates);

    std::shared_ptr<RatesEnvironment> getRatesEnvironmentForEuropeanOption(QuantLib::Date anchorDate,
                                                                             std::vector<QuantLib::Date> manditoryDates);

    std::shared_ptr<RatesEnvironment> getRatesEnvironmentForAsianOption(QuantLib::Date anchorDate,
                                                                          std::vector<QuantLib::Date> manditoryDates);






    // Generates fixing points using the forward curve
    std::shared_ptr<DailyFixingCurve> buildFixingCurve(std::shared_ptr<ForwardCurveDeterministic> forwardCurve,
                                                               std::vector<QuantLib::Date> fixingDates);

    std::shared_ptr<DailyFixingCurve> buildFixingCurve(double fixedRates,
                                                               std::vector<QuantLib::Date> fixingDates);

    std::vector<QuantLib::Date> getDatesVector(std::shared_ptr<QuantLib::Calendar> calendar, QuantLib::Date anchorDate, size_t years, std::vector<QuantLib::Date> manditoryDates = std::vector<QuantLib::Date>());


    std::shared_ptr<QuantLib::DayCounter> defaultDayCounter;
    std::shared_ptr<QuantLib::Calendar> defaultCalendar;
    QuantLib::Date defaultAnchorDate;
    size_t defaultYears;
    double defaultNACCRate;
    double defaultNACCIRDiff;
    double defaultSpot;
    double defaultVolatiltiy;
    std::vector<QuantLib::Date> defaultCurveDates, defaultSurfaceDates;
    std::vector<double> defaultMoneyness;
    std::vector<std::vector<double>> defaultVolatilitySurface;


};

#endif

