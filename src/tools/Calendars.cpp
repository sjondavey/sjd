#include <sjd/tools/Calendars.h>
#include <sjd/tools/Errors.h>

#include <functional>
#include <map>

#include <ql/time/calendars/australia.hpp>
#include <ql/time/calendars/canada.hpp>
#include <ql/time/calendars/china.hpp>
#include <ql/time/calendars/germany.hpp>
#include <ql/time/calendars/hongkong.hpp>
#include <ql/time/calendars/india.hpp>
#include <ql/time/calendars/japan.hpp>
#include <ql/time/calendars/jointcalendar.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/calendars/singapore.hpp>
#include <ql/time/calendars/southafrica.hpp>
#include <ql/time/calendars/switzerland.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/calendars/unitedkingdom.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/time/calendars/weekendsonly.hpp>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    namespace
    {
        const map<string, function<Calendar()>>& registry()
        {
            static const map<string, function<Calendar()>> calendars = {
                {"Australia", [] { return Australia(); }},
                {"Canada", [] { return Canada(Canada::Settlement); }},
                {"Canada.TSX", [] { return Canada(Canada::TSX); }},
                {"China", [] { return China(China::SSE); }},
                {"China.IB", [] { return China(China::IB); }},
                {"Germany", [] { return Germany(Germany::Settlement); }},
                {"Germany.Eurex", [] { return Germany(Germany::Eurex); }},
                {"HongKong", [] { return HongKong(); }},
                {"India", [] { return India(); }},
                {"Japan", [] { return Japan(); }},
                {"NullCalendar", [] { return NullCalendar(); }},
                {"Singapore", [] { return Singapore(); }},
                {"SouthAfrica", [] { return SouthAfrica(); }},
                {"Switzerland", [] { return Switzerland(); }},
                {"TARGET", [] { return TARGET(); }},
                {"UnitedKingdom", [] { return UnitedKingdom(UnitedKingdom::Settlement); }},
                {"UnitedKingdom.Exchange", [] { return UnitedKingdom(UnitedKingdom::Exchange); }},
                {"UnitedKingdom.Metals", [] { return UnitedKingdom(UnitedKingdom::Metals); }},
                {"UnitedStates", [] { return UnitedStates(UnitedStates::Settlement); }},
                {"UnitedStates.FederalReserve", [] { return UnitedStates(UnitedStates::FederalReserve); }},
                {"UnitedStates.GovernmentBond", [] { return UnitedStates(UnitedStates::GovernmentBond); }},
                {"UnitedStates.NYSE", [] { return UnitedStates(UnitedStates::NYSE); }},
                {"UnitedStates.SOFR", [] { return UnitedStates(UnitedStates::SOFR); }},
                {"WeekendsOnly", [] { return WeekendsOnly(); }},
            };
            return calendars;
        }
    }

    Calendar calendarNamed(const string& name)
    {
        auto found = registry().find(name);
        if (found == registry().end())
        {
            throw ConfigurationError("Unknown calendar \"" + name + "\"");
        }
        return found->second();
    }

    Calendar jointCalendar(const vector<string>& components)
    {
        if (components.empty())
        {
            throw ConfigurationError("A joint calendar needs at least one component");
        }
        if (components.size() == 1)
        {
            return calendarNamed(components[0]);
        }
        vector<Calendar> calendars;
        for (const string& component : components)
        {
            calendars.push_back(calendarNamed(component));
        }
        return JointCalendar(calendars, JoinHolidays);
    }

    vector<string> calendarNames()
    {
        vector<string> names;
        for (const auto& entry : registry())
        {
            names.push_back(entry.first);
        }
        return names;
    }
}
