#pragma once

#include <ql/time/date.hpp>
#include <string>
#include <vector>

// BuySell and PutCall, shared by the contracts.

namespace sjd
{

    enum BuySell
    {
        BUY,
        SELL
    };
    enum PutCall
    {
        PUT,
        CALL
    };

    std::string convertBuySellToString(BuySell bs);

    std::string convertPutCallToString(PutCall pc);
}
