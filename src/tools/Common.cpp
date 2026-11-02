#include <sjd/tools/Common.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    string convertBuySellToString(BuySell bs)
    {
        string output;
        switch (bs)
        {
        case BuySell::BUY:
        {
            output = "BUY";
        }
        break;
        case BuySell::SELL:
        {
            output = "SELL";
        }
        break;
        default:
            output = "Buy/Sell not defined";
        }
        return output;
    }

    string convertPutCallToString(PutCall pc)
    {
        string output;
        switch (pc)
        {
        case PutCall::PUT:
        {
            output = "PUT";
        }
        break;
        case PutCall::CALL:
        {
            output = "CALL";
        }
        break;
        default:
            output = "Put/Call not defined";
        }
        return output;
    }

}