#pragma once

#include <ql/time/date.hpp>
#include <string>
#include <vector>

// BuySell and PutCall, shared by the contracts.

namespace sjd
{

    enum class BuySell
    {
        BUY,
        SELL
    };
    enum class PutCall
    {
        PUT,
        CALL
    };

    // +1 for a buy, -1 for a sell.
    inline double sign(BuySell buySell)
    {
        return buySell == BuySell::BUY ? 1.0 : -1.0;
    }

    std::string convertBuySellToString(BuySell bs);

    std::string convertPutCallToString(PutCall pc);
}
