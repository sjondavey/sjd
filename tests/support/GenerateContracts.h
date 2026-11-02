#pragma once

#include <iostream>

#include <sjd/contracts/AsianForwardContract.h>
#include <sjd/contracts/AsianOptionContract.h>
#include <sjd/contracts/Cashflow.h>
#include <sjd/contracts/EuropeanForwardContract.h>
#include <sjd/contracts/EuropeanOptionContract.h>

#include <ql/time/calendars/all.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/all.hpp>

using namespace sjd;

class GenerateContracts
{
public:
    static std::shared_ptr<FixedCashflow> getDefaultFixedCashflow(QuantLib::Date anchorDate,
                                                                  int yearsToSettlement,
                                                                  double volume,
                                                                  BuySell bs);

    static std::shared_ptr<EuropeanForward> getDefaultEuropeanForward(
        QuantLib::Date anchorDate, int yearsToSettlement, double strike, double volume, BuySell bs);
    static std::shared_ptr<EuropeanOption> getDefaultEuropeanOption(QuantLib::Date anchorDate,
                                                                    int yearsToSettlement,
                                                                    double strike,
                                                                    double volume,
                                                                    BuySell bs,
                                                                    PutCall pc,
                                                                    double premium,
                                                                    bool dp = false);

    static std::shared_ptr<AsianForward> getDefaultAsianForward(QuantLib::Date anchorDate,
                                                                int yearsToSettlement,
                                                                int numberOfAveragingDates,
                                                                double strike,
                                                                double volume,
                                                                BuySell bs);
    static std::shared_ptr<AsianOption> getDefaultAsianOption(QuantLib::Date anchorDate,
                                                              int yearsToSettlement,
                                                              int numberOfAveragingDates,
                                                              double strike,
                                                              double volume,
                                                              BuySell bs,
                                                              PutCall pc,
                                                              double premium,
                                                              bool dp = false);

    static std::shared_ptr<FixedCashflow> getDefaultFixedCashflowWithPremium(QuantLib::Date anchorDate,
                                                                             int yearsToSettlement,
                                                                             double volume,
                                                                             BuySell bs);
};
