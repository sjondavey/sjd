#include "GenerateContracts.h"

using namespace std;
using namespace QuantLib;
using namespace boost;

using namespace sjd;

std::shared_ptr<FixedCashflow> GenerateContracts::getDefaultFixedCashflow(Date anchorDate,
                                                                          int yearsToSettlement,
                                                                          double volume,
                                                                          BuySell bs)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date cashFlowDate = nullCalendar->advance(anchorDate, yearsToSettlement, Years);
    Date premiumDate = cashFlowDate;

    return std::shared_ptr<FixedCashflow>(new FixedCashflow(cashFlowDate, volume, bs));
}

std::shared_ptr<EuropeanForward> GenerateContracts::getDefaultEuropeanForward(
    Date anchorDate, int yearsToSettlement, double strike, double volume, BuySell bs)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date settlement = nullCalendar->advance(anchorDate, yearsToSettlement, Years);
    Date rateSetDate = nullCalendar->advance(settlement, -2, Days);

    return std::shared_ptr<EuropeanForward>(
        new EuropeanForward(rateSetDate, settlement, abs(strike), abs(volume), bs));
}

std::shared_ptr<AsianForward> GenerateContracts::getDefaultAsianForward(Date anchorDate,
                                                                        int yearsToSettlement,
                                                                        int numberOfAveragingDates,
                                                                        double strike,
                                                                        double volume,
                                                                        BuySell bs)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date settlement = nullCalendar->advance(anchorDate, yearsToSettlement, Years);
    Date d = nullCalendar->advance(settlement, -5, Days);
    set<Date> averagingDates;
    unsigned int total = (unsigned int)abs(numberOfAveragingDates);
    for (size_t i = 0; i < total; ++i)
    {
        averagingDates.insert(d);
        d = nullCalendar->advance(d, -1, Days);
    }
    vector<Date> averagingDatesAsVector = vector<Date>(averagingDates.begin(), averagingDates.end());

    return std::shared_ptr<AsianForward>(new AsianForward(averagingDates, settlement, strike, volume, bs));
}

std::shared_ptr<EuropeanOption> GenerateContracts::getDefaultEuropeanOption(Date anchorDate,
                                                                            int yearsToSettlement,
                                                                            double strike,
                                                                            double volume,
                                                                            BuySell bs,
                                                                            PutCall pc,
                                                                            double premium,
                                                                            bool dp)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date settlement = nullCalendar->advance(anchorDate, yearsToSettlement, Years);
    Date rateSetDate = nullCalendar->advance(settlement, -2, Days);

    Date dpDate;
    if (dp == true)
        dpDate = settlement;
    else
        dpDate = anchorDate;

    return std::shared_ptr<EuropeanOption>(
        new EuropeanOption(rateSetDate, settlement, strike, pc, volume, dpDate, premium, bs));
}

std::shared_ptr<AsianOption> GenerateContracts::getDefaultAsianOption(Date anchorDate,
                                                                      int yearsToSettlement,
                                                                      int numberOfAveragingDates,
                                                                      double strike,
                                                                      double volume,
                                                                      BuySell bs,
                                                                      PutCall pc,
                                                                      double premium,
                                                                      bool dp)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date settlement = nullCalendar->advance(anchorDate, yearsToSettlement, Years);
    Date d = nullCalendar->advance(settlement, -5, Days);
    set<Date> averagingDates;
    unsigned int total = (unsigned int)abs(numberOfAveragingDates);
    for (size_t i = 0; i < total; ++i)
    {
        averagingDates.insert(d);
        d = nullCalendar->advance(d, -1, Days);
    }
    vector<Date> averagingDatesAsVector = vector<Date>(averagingDates.begin(), averagingDates.end());

    Date dpDate;
    if (dp == true)
    {
        dpDate = settlement;
    }
    else
    {
        dpDate = anchorDate;
    }

    return std::shared_ptr<AsianOption>(
        new AsianOption(averagingDates, settlement, strike, pc, volume, dpDate, premium, bs));
}

std::shared_ptr<FixedCashflow> GenerateContracts::getDefaultFixedCashflowWithPremium(Date anchorDate,
                                                                                     int yearsToSettlement,
                                                                                     double volume,
                                                                                     BuySell bs)
{
    std::shared_ptr<Calendar> nullCalendar = std::shared_ptr<Calendar>(new NullCalendar());
    Date cashFlowDate = nullCalendar->advance(anchorDate, yearsToSettlement,
                                              Years); // I want this to have the same settlement as the other
                                                      // ontracts if I intput the same year fraction
    Date maturity = nullCalendar->advance(cashFlowDate, -2, Days);

    std::shared_ptr<FixedCashflow> fcf;
    Date dpDate;
    dpDate = anchorDate;
    fcf = std::shared_ptr<FixedCashflow>(new FixedCashflow(cashFlowDate, volume, bs));
    return fcf;
}
