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
        case BUY:
        {
            output = "BUY";
        }
        break;
        case SELL:
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
        case PUT:
        {
            output = "PUT";
        }
        break;
        case CALL:
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