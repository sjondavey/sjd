#ifndef SJD_CALENDARS_INCLUDED
#define SJD_CALENDARS_INCLUDED

#pragma once

#include <ql/time/calendar.hpp>
#include <string>
#include <vector>

namespace sjd
{
    // Names are the QuantLib class, then an optional ".Market" for calendars with several
    // markets, e.g. "UnitedKingdom", "UnitedKingdom.Metals", "UnitedStates.NYSE". A class
    // name on its own means its settlement calendar. Throws std::invalid_argument for a
    // name that is not in calendarNames().
    QuantLib::Calendar calendarNamed(const std::string& name);

    // A day is a business day only when it is one in every component.
    QuantLib::Calendar jointCalendar(const std::vector<std::string>& components);

    std::vector<std::string> calendarNames();
}

#endif
