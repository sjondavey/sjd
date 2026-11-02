#ifndef TOOLS_INCLUDED
#define TOOLS_INCLUDED

#pragma once

#include <boost/lexical_cast.hpp>
#include <ql/time/date.hpp>
#include <string>
#include <vector>

/*======================================================================================
Overview
The most basic financial instument is the Contract. When building and, more importantly,
testing a Contract we focus on the the contracts "value" at different points in time.
Once we are convinced that the "value" is correct we can build in additional important
functionality by implementing various interfaces.

Structure let's us implement the regulatory capital calculation for one contract or a
logical group of contracts

Portfolio lets us group Structures and perform grouped analysis
=======================================================================================*/

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

#endif