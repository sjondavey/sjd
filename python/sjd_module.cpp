#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <set>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <cstdio>
#include <cstdlib>
#endif

#include <memory>
#include <ql/time/date.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>

#include <sjd/pricing/Black76Formula.h>
#include <sjd/rates/Forward/ForwardCurveFactory.h>
#include <sjd/rates/Discount/DiscountCurves.h>
#include <sjd/rates/Fixing/FixingCurves.h>
#include <sjd/rates/Volatility/VolatilityCurveCMA.h>
#include <sjd/rates/Volatility/VolatilityCurve.h>
#include <sjd/rates/Volatility/VolatilitySurfaceDelta.h>
#include <sjd/rates/Volatility/VolatilitySurfaceMoneynessCMA.h>
#include <sjd/rates/RatesEnvironment.h>
#include <sjd/contracts/AsianForwardContract.h>
#include <sjd/contracts/AsianOptionContract.h>
#include <sjd/contracts/EuropeanForwardContract.h>
#include <sjd/contracts/EuropeanOptionContract.h>
#include <sjd/contracts/Portfolio/Margin/Margin.h>
#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/contracts/Portfolio/PortfolioEvolver.h>
#include <sjd/contracts/Portfolio/RegulatoryCapital.h>
#include <sjd/rates/PathGenerators/SingleFactorGMB.h>
#include <sjd/tools/Calendars.h>
#include <sjd/tools/Common.h>
#include <sjd/maths/maths.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace py = pybind11;

namespace {

#if defined(_MSC_VER) && defined(_DEBUG)
void exit_on_invalid_parameter(const wchar_t* expression,
                               const wchar_t*,
                               const wchar_t* file,
                               unsigned int line,
                               uintptr_t)
{
    std::fwprintf(stderr,
                  L"sjd: debug CRT check failed: %ls (%ls:%u)\n",
                  expression ? expression : L"?",
                  file ? file : L"?",
                  line);
    std::fflush(stderr);
    std::_Exit(3);
}
#endif

// A debug-build check (such as a vector index out of range) normally opens a modal
// dialog, which blocks unattended test runs. With SJD_NO_DEBUG_DIALOGS set, the check is
// written to stderr and the process exits with code 3 instead.
void report_debug_checks_to_stderr()
{
#if defined(_MSC_VER) && defined(_DEBUG)
    if (!std::getenv("SJD_NO_DEBUG_DIALOGS"))
        return;
    for (int type : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT})
    {
        _CrtSetReportMode(type, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
    }
    _set_invalid_parameter_handler(exit_on_invalid_parameter);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
}

QuantLib::Date to_ql_date(const py::object& value)
{
    if (value.is_none())
        throw py::type_error("date is None");
    const int year = value.attr("year").cast<int>();
    const int month = value.attr("month").cast<int>();
    const int day = value.attr("day").cast<int>();
    return QuantLib::Date(day, static_cast<QuantLib::Month>(month), year);
}

std::vector<QuantLib::Date> to_ql_dates(const py::iterable& values)
{
    std::vector<QuantLib::Date> dates;
    for (const py::handle& item : values)
        dates.push_back(to_ql_date(py::reinterpret_borrow<py::object>(item)));
    return dates;
}

py::object from_ql_date(const QuantLib::Date& date)
{
    return py::module_::import("datetime").attr("date")(
        static_cast<int>(date.year()),
        static_cast<int>(date.month()),
        date.dayOfMonth());
}

std::shared_ptr<QuantLib::DayCounter> actual365()
{
    return std::shared_ptr<QuantLib::DayCounter>(new QuantLib::Actual365Fixed());
}

std::vector<std::shared_ptr<sjd::SingleInstrumentContract>> contracts_from(const py::list& contracts)
{
    std::vector<std::shared_ptr<sjd::SingleInstrumentContract>> inputs;
    for (const py::handle& item : contracts)
    {
        if (!py::isinstance<sjd::SingleInstrumentContract>(item))
        {
            throw py::type_error(
                "Portfolio contracts must be single instruments, such as EuropeanOption "
                "or AsianForward, not " + std::string(py::str(py::type::of(item).attr("__name__"))));
        }
        inputs.push_back(item.cast<std::shared_ptr<sjd::SingleInstrumentContract>>());
    }
    return inputs;
}

std::set<QuantLib::Date> simulation_dates_from(const py::iterable& values)
{
    std::set<QuantLib::Date> dates;
    for (const QuantLib::Date& date : to_ql_dates(values))
    {
        dates.insert(date);
    }
    return dates;
}

std::vector<std::shared_ptr<sjd::Contract>> contract_inputs_from(const py::list& contracts)
{
    std::vector<std::shared_ptr<sjd::Contract>> inputs;
    for (const std::shared_ptr<sjd::SingleInstrumentContract>& contract : contracts_from(contracts))
    {
        inputs.push_back(contract);
    }
    return inputs;
}

py::dict detail_table(const std::vector<QuantLib::Date>& dates,
                      const std::vector<std::string>& headings,
                      const std::vector<std::vector<double>>& rows)
{
    py::list py_dates;
    for (const QuantLib::Date& date : dates)
        py_dates.append(from_ql_date(date));
    py::list py_rows;
    for (const std::vector<double>& row : rows)
        py_rows.append(py::cast(row));
    py::dict table;
    table["dates"] = py_dates;
    table["headings"] = headings;
    table["rows"] = py_rows;
    return table;
}

struct NamedCalendar
{
    std::string name;
    std::vector<std::string> components;
    QuantLib::Calendar calendar;
};

py::list py_dates(const std::vector<QuantLib::Date>& dates)
{
    py::list result;
    for (const QuantLib::Date& date : dates)
        result.append(from_ql_date(date));
    return result;
}

std::shared_ptr<sjd::PortfolioEvolver> make_evolver(
    std::shared_ptr<sjd::Portfolio> portfolio,
    std::shared_ptr<sjd::SingleFactorEconomicScenarioGenerator> generator,
    const py::iterable& simulation_dates,
    std::shared_ptr<sjd::RegulatoryCapital> capital,
    size_t holding_period,
    const py::object& holding_period_calendar)
{
    const std::set<QuantLib::Date> dates = simulation_dates_from(simulation_dates);
    if (holding_period == 0)
        return std::shared_ptr<sjd::PortfolioEvolver>(
            new sjd::PortfolioEvolver(portfolio, generator, dates, capital));
    if (holding_period_calendar.is_none())
        throw py::value_error("holding_period needs a holding_period_calendar");
    std::shared_ptr<QuantLib::Calendar> calendar(
        new QuantLib::Calendar(holding_period_calendar.cast<const NamedCalendar&>().calendar));
    return std::shared_ptr<sjd::PortfolioEvolver>(
        new sjd::PortfolioEvolver(portfolio, generator, dates, calendar, holding_period, capital));
}

struct Simulation
{
    py::list dates;
    py::array_t<py::ssize_t> exposure_steps;
    py::array_t<py::ssize_t> margin_reset_steps;
    py::array_t<double> spot;
    py::array_t<double> mtm;
    py::object margin;
    py::object by_trade;
    size_t paths = 0;
    size_t steps = 0;
    size_t trades = 0;
    bool antithetic = false;
};

py::array_t<double> to_array(const std::vector<double>& values, const std::vector<py::ssize_t>& shape)
{
    py::array_t<double> array(shape);
    std::copy(values.begin(), values.end(), array.mutable_data());
    return array;
}

py::array_t<py::ssize_t> to_index_array(const std::vector<size_t>& values)
{
    py::array_t<py::ssize_t> array(static_cast<py::ssize_t>(values.size()));
    std::transform(values.begin(), values.end(), array.mutable_data(),
                   [](size_t value) { return static_cast<py::ssize_t>(value); });
    return array;
}

Simulation simulation_from(const sjd::SimulatedPaths& simulated)
{
    Simulation result;
    for (const QuantLib::Date& date : simulated.dates)
        result.dates.append(from_ql_date(date));
    result.exposure_steps = to_index_array(simulated.exposureSteps);
    result.margin_reset_steps = to_index_array(simulated.marginResetSteps);
    const py::ssize_t paths = static_cast<py::ssize_t>(simulated.paths);
    const py::ssize_t steps = static_cast<py::ssize_t>(simulated.steps);
    result.spot = to_array(simulated.spot, {paths, steps});
    result.mtm = to_array(simulated.mtm, {paths, steps});
    result.margin = simulated.margin.empty()
        ? py::none()
        : py::object(to_array(simulated.margin, {paths, steps}));
    result.by_trade = simulated.byTrade.empty()
        ? py::none()
        : py::object(to_array(simulated.byTrade,
                              {paths, steps, static_cast<py::ssize_t>(simulated.trades)}));
    result.paths = simulated.paths;
    result.steps = simulated.steps;
    result.trades = simulated.trades;
    result.antithetic = simulated.antithetic;
    return result;
}

} // namespace

PYBIND11_MODULE(_core, m)
{
    report_debug_checks_to_stderr();
    m.doc() = "sjd derivative pricing (compiled core; import sjd rather than sjd._core)";

    py::enum_<sjd::BuySell>(m, "BuySell")
        .value("BUY", sjd::BUY)
        .value("SELL", sjd::SELL);

    py::enum_<sjd::PutCall>(m, "PutCall")
        .value("PUT", sjd::PUT)
        .value("CALL", sjd::CALL);

    py::enum_<QuantLib::BusinessDayConvention>(m, "BusinessDayConvention")
        .value("FOLLOWING", QuantLib::Following)
        .value("MODIFIED_FOLLOWING", QuantLib::ModifiedFollowing)
        .value("PRECEDING", QuantLib::Preceding)
        .value("MODIFIED_PRECEDING", QuantLib::ModifiedPreceding)
        .value("UNADJUSTED", QuantLib::Unadjusted);

    py::enum_<QuantLib::TimeUnit>(m, "TimeUnit")
        .value("DAYS", QuantLib::Days)
        .value("WEEKS", QuantLib::Weeks)
        .value("MONTHS", QuantLib::Months)
        .value("YEARS", QuantLib::Years);

    py::class_<NamedCalendar>(m, "Calendar",
                              "A QuantLib holiday calendar. With components, a joint calendar "
                              "that is closed when any component is closed, and name is only "
                              "a label.")
        .def(py::init([](const std::string& name, const std::vector<std::string>& components) {
                 NamedCalendar result;
                 result.name = name;
                 result.components = components;
                 result.calendar = components.empty() ? sjd::calendarNamed(name)
                                                      : sjd::jointCalendar(components);
                 return result;
             }),
             py::arg("name"),
             py::arg("components") = std::vector<std::string>())
        .def_readonly("name", &NamedCalendar::name)
        .def_property_readonly("components", [](const NamedCalendar& calendar) {
            return py::tuple(py::cast(calendar.components));
        })
        .def_static("names", &sjd::calendarNames, "Calendar names accepted by Calendar().")
        .def("is_business_day", [](const NamedCalendar& calendar, const py::object& day) {
            return calendar.calendar.isBusinessDay(to_ql_date(day));
        }, py::arg("day"))
        .def("is_holiday", [](const NamedCalendar& calendar, const py::object& day) {
            return calendar.calendar.isHoliday(to_ql_date(day));
        }, py::arg("day"), "True on weekends as well as public holidays.")
        .def("adjust", [](const NamedCalendar& calendar,
                          const py::object& day,
                          QuantLib::BusinessDayConvention convention) {
            return from_ql_date(calendar.calendar.adjust(to_ql_date(day), convention));
        }, py::arg("day"), py::arg("convention") = QuantLib::Following)
        .def("advance", [](const NamedCalendar& calendar,
                           const py::object& day,
                           int count,
                           QuantLib::TimeUnit unit,
                           QuantLib::BusinessDayConvention convention,
                           bool end_of_month) {
            return from_ql_date(calendar.calendar.advance(
                to_ql_date(day), count, unit, convention, end_of_month));
        },
             py::arg("day"),
             py::arg("count"),
             py::arg("unit") = QuantLib::Days,
             py::arg("convention") = QuantLib::Following,
             py::arg("end_of_month") = false,
             "Move count business days with unit DAYS. Other units move the calendar "
             "period and then adjust by convention.")
        .def("business_days_between", [](const NamedCalendar& calendar,
                                         const py::object& start,
                                         const py::object& end,
                                         bool include_first,
                                         bool include_last) {
            return calendar.calendar.businessDaysBetween(
                to_ql_date(start), to_ql_date(end), include_first, include_last);
        },
             py::arg("start"),
             py::arg("end"),
             py::arg("include_first") = true,
             py::arg("include_last") = false)
        .def("holidays", [](const NamedCalendar& calendar,
                            const py::object& start,
                            const py::object& end,
                            bool include_weekends) {
            return py_dates(calendar.calendar.holidayList(
                to_ql_date(start), to_ql_date(end), include_weekends));
        },
             py::arg("start"),
             py::arg("end"),
             py::arg("include_weekends") = false)
        .def("__repr__", [](const NamedCalendar& calendar) {
            return "<Calendar " + calendar.name + ">";
        });

    py::enum_<sjd::ValidationSeverity>(m, "ValidationSeverity")
        .value("INFO", sjd::ValidationSeverity::INFO)
        .value("WARNING", sjd::ValidationSeverity::WARNING)
        .value("ERROR", sjd::ValidationSeverity::ERROR);

    py::class_<sjd::ValidationIssue>(m, "ValidationIssue")
        .def_readonly("code", &sjd::ValidationIssue::code)
        .def_readonly("message", &sjd::ValidationIssue::message)
        .def_readonly("severity", &sjd::ValidationIssue::severity)
        .def_property_readonly("path", [](const sjd::ValidationIssue& issue) {
            py::tuple path(issue.path.size());
            for (size_t i = 0; i < issue.path.size(); ++i)
                path[i] = issue.path[i];
            return path;
        });

    py::class_<sjd::ValidationReport>(m, "ValidationReport")
        .def_property_readonly("ok", &sjd::ValidationReport::isOK)
        .def_property_readonly("issues", [](const sjd::ValidationReport& report) {
            const std::vector<sjd::ValidationIssue>& source = report.getIssues();
            py::tuple issues(source.size());
            for (size_t i = 0; i < source.size(); ++i)
                issues[i] = py::cast(source[i]);
            return issues;
        });

    py::class_<sjd::Black76Call>(m, "Black76Call")
        .def(py::init<double, double, double, double>(),
             py::arg("forward"),
             py::arg("strike"),
             py::arg("standard_deviation"),
             py::arg("discount_factor"))
        .def("is_ok", &sjd::Black76Call::isOK)
        .def("error_message", &sjd::Black76Call::getErrorMessage)
        .def("premium", &sjd::Black76Call::getPremium)
        .def("delta", &sjd::Black76Call::getDelta);

    py::class_<sjd::Black76Put>(m, "Black76Put")
        .def(py::init<double, double, double, double>(),
             py::arg("forward"),
             py::arg("strike"),
             py::arg("standard_deviation"),
             py::arg("discount_factor"))
        .def("is_ok", &sjd::Black76Put::isOK)
        .def("error_message", &sjd::Black76Put::getErrorMessage)
        .def("premium", &sjd::Black76Put::getPremium)
        .def("delta", &sjd::Black76Put::getDelta);

    py::enum_<sjd::ArrayInterpolatorType>(m, "CurveInterpolator")
        .value("LINEAR", sjd::LINEAR)
        .value("CUBIC", sjd::CUBIC);

    py::enum_<sjd::ForwardAveraging>(m, "ForwardAveraging")
        .value("POINT", sjd::ForwardAveraging::Point)
        .value("CALENDAR_MONTH", sjd::ForwardAveraging::CalendarMonth);

    py::enum_<sjd::ForwardRoll>(m, "ForwardRoll")
        .value("DETERMINISTIC", sjd::ForwardRoll::Deterministic)
        .value("SPOT_VOLATILITY", sjd::ForwardRoll::SpotVolatility)
        .value("FORWARD_VOLATILITY", sjd::ForwardRoll::ForwardVolatility);

    // Base for every volatility curve and surface, so any of them can be passed
    // where a volatility source is expected.
    py::class_<sjd::VolatilityRateSource, std::shared_ptr<sjd::VolatilityRateSource>>(
        m, "VolatilityRateSource")
        .def("is_ok", &sjd::VolatilityRateSource::isOK)
        .def("error_messages", &sjd::VolatilityRateSource::getErrorMessagesAsString)
        .def("anchor_date", [](const sjd::VolatilityRateSource& source) {
            return from_ql_date(source.getAnchorDate());
        });

    py::class_<sjd::ForwardRateSource, std::shared_ptr<sjd::ForwardRateSource>>(m, "ForwardCurve")
        .def(py::init([](const py::object& anchor,
                         const py::iterable& observation_dates,
                         const std::vector<double>& forwards,
                         bool allow_extrapolation,
                         sjd::ForwardAveraging averaging,
                         sjd::ForwardRoll roll,
                         sjd::ArrayInterpolatorType interpolation,
                         std::shared_ptr<sjd::VolatilityRateSource> volatility) {
                 try
                 {
                     return sjd::makeForwardCurve(to_ql_date(anchor),
                                                  to_ql_dates(observation_dates),
                                                  forwards,
                                                  interpolation,
                                                  allow_extrapolation,
                                                  averaging,
                                                  roll,
                                                  volatility);
                 }
                 catch (const std::invalid_argument& error)
                 {
                     throw py::value_error(error.what());
                 }
             }),
             py::arg("anchor_date"),
             py::arg("observation_dates"),
             py::arg("forwards"),
             py::arg("allow_extrapolation") = false,
             py::kw_only(),
             py::arg("averaging") = sjd::ForwardAveraging::Point,
             py::arg("roll") = sjd::ForwardRoll::Deterministic,
             py::arg("interpolation") = sjd::LINEAR,
             py::arg("volatility") = py::none(),
             "Interpolated forward curve.\n\n"
             "averaging=CALENDAR_MONTH reads observation dates by month and year only.\n"
             "roll=DETERMINISTIC ignores volatility. SPOT_VOLATILITY and FORWARD_VOLATILITY "
             "need a volatility curve or surface, and PortfolioEvolver needs one of them. "
             "Rolled curves interpolate linearly whatever the interpolation here.")
        .def("is_ok", &sjd::ForwardRateSource::isOK)
        .def("error_messages", &sjd::ForwardRateSource::getErrorMessagesAsString)
        .def("validation_report", &sjd::ForwardRateSource::validationReport)
        .def("validate_report", &sjd::ForwardRateSource::validateReport)
        .def("anchor_date", [](const sjd::ForwardRateSource& curve) {
            return from_ql_date(curve.getAnchorDate());
        })
        .def("final_date", [](const sjd::ForwardRateSource& curve) {
            return from_ql_date(curve.getFinalDate());
        })
        .def("get_forward", [](const sjd::ForwardRateSource& curve, const py::object& to) {
            return curve.getForward(to_ql_date(to));
        });

    py::class_<sjd::DiscountCurve, std::shared_ptr<sjd::DiscountCurve>>(m, "DiscountCurve")
        .def(py::init([](const py::object& anchor,
                         const py::iterable& observation_dates,
                         const std::vector<double>& discount_factors,
                         bool allow_extrapolation) {
                 return std::shared_ptr<sjd::DiscountCurve>(
                     new sjd::DiscountCurve(
                         to_ql_date(anchor),
                         to_ql_dates(observation_dates),
                         discount_factors,
                         sjd::LINEAR,
                         allow_extrapolation));
             }),
             py::arg("anchor_date"),
             py::arg("observation_dates"),
             py::arg("discount_factors"),
             py::arg("allow_extrapolation") = false)
        .def("is_ok", &sjd::DiscountCurve::isOK)
        .def("error_messages", &sjd::DiscountCurve::getErrorMessagesAsString)
        .def("get_discount_factor", [](const sjd::DiscountCurve& curve, const py::object& to) {
            return curve.getDiscountFactor(to_ql_date(to));
        });

    py::class_<sjd::VolatilityCurve, sjd::VolatilityRateSource, std::shared_ptr<sjd::VolatilityCurve>>(
        m, "VolatilityCurve")
        .def(py::init([](const py::object& anchor,
                         const py::iterable& observation_dates,
                         const std::vector<double>& volatilities,
                         bool allow_extrapolation) {
                 return std::shared_ptr<sjd::VolatilityCurve>(
                     new sjd::VolatilityCurve(
                         to_ql_date(anchor),
                         to_ql_dates(observation_dates),
                         volatilities,
                         actual365(),
                         sjd::LINEAR,
                         allow_extrapolation));
             }),
             py::arg("anchor_date"),
             py::arg("observation_dates"),
             py::arg("volatilities"),
             py::arg("allow_extrapolation") = false,
             "Volatilities are interpolated in standard-deviation space. "
             "Year fractions use Actual/365 Fixed (not a QuantLib-Python type).")
        .def("is_ok", &sjd::VolatilityCurve::isOK)
        .def("error_messages", &sjd::VolatilityCurve::getErrorMessagesAsString)
        .def("get_volatility", [](const sjd::VolatilityCurve& curve, const py::object& to) {
            return curve.getVolatility(to_ql_date(to));
        });

    py::class_<sjd::VolatilityCurveCMA, sjd::VolatilityRateSource, std::shared_ptr<sjd::VolatilityCurveCMA>>(
        m, "VolatilityCurveCMA")
        .def(py::init([](const py::object& anchor,
                         const py::iterable& observation_dates,
                         const std::vector<double>& volatilities,
                         bool allow_extrapolation) {
                 return std::shared_ptr<sjd::VolatilityCurveCMA>(
                     new sjd::VolatilityCurveCMA(
                         to_ql_date(anchor),
                         to_ql_dates(observation_dates),
                         volatilities,
                         actual365(),
                         sjd::LINEAR,
                         allow_extrapolation));
             }),
             py::arg("anchor_date"),
             py::arg("observation_dates"),
             py::arg("volatilities"),
             py::arg("allow_extrapolation") = false,
             "Calendar-month-average volatility curve using Actual/365 Fixed.")
        .def("is_ok", &sjd::VolatilityCurveCMA::isOK)
        .def("error_messages", &sjd::VolatilityCurveCMA::getErrorMessagesAsString)
        .def("get_volatility", [](const sjd::VolatilityCurveCMA& curve,
                                  const py::object& to) {
            return curve.getVolatility(to_ql_date(to));
        })
        .def("get_standard_deviation", [](const sjd::VolatilityCurveCMA& curve,
                                          const py::object& to) {
            return curve.getStandardDeviation(to_ql_date(to));
        });

    py::class_<sjd::DailyFixingCurve, std::shared_ptr<sjd::DailyFixingCurve>>(
        m, "DailyFixingCurve")
        .def(py::init([](const py::iterable& fixing_dates,
                         const std::vector<double>& fixing_rates) {
                 return std::shared_ptr<sjd::DailyFixingCurve>(
                     new sjd::DailyFixingCurve(
                         to_ql_dates(fixing_dates),
                         fixing_rates));
             }),
             py::arg("fixing_dates"),
             py::arg("fixing_rates"))
        .def("is_ok", &sjd::DailyFixingCurve::isOK)
        .def("error_messages", &sjd::DailyFixingCurve::getErrorMessages)
        .def("get_rate", [](const sjd::DailyFixingCurve& curve,
                            const py::object& on_date) {
            return curve.getRateSetOn(to_ql_date(on_date));
        });

    py::enum_<SurfaceInterpolatorType>(m, "SurfaceInterpolator")
        .value("BILINEAR", BILINEAR)
        .value("BICUBIC", BICUBIC);

    py::class_<sjd::VolatilitySurfaceDelta, sjd::VolatilityRateSource, std::shared_ptr<sjd::VolatilitySurfaceDelta>>(
        m, "VolatilitySurfaceDelta")
        .def(py::init([](const py::object& anchor,
                         const py::iterable& observation_dates,
                         const std::vector<double>& deltas,
                         const std::vector<std::vector<double>>& volatility,
                         SurfaceInterpolatorType interpolator,
                         bool extrapolate) {
                 return std::shared_ptr<sjd::VolatilitySurfaceDelta>(
                     new sjd::VolatilitySurfaceDelta(
                         to_ql_date(anchor),
                         to_ql_dates(observation_dates),
                         deltas,
                         volatility,
                         interpolator,
                         extrapolate));
             }),
             py::arg("anchor_date"),
             py::arg("observation_dates"),
             py::arg("deltas"),
             py::arg("volatility"),
             py::arg("interpolator"),
             py::arg("extrapolate") = false,
             "Row of volatility is a put delta in percent. Column is an observation date. "
             "Deltas outside the grid read the nearest wing; extrapolate only extends time.")
        .def("is_ok", &sjd::VolatilitySurfaceDelta::isOK)
        .def("error_messages", &sjd::VolatilitySurfaceDelta::getErrorMessagesAsString)
        .def("get_volatility", [](const sjd::VolatilitySurfaceDelta& surface,
                                  const py::object& to,
                                  double strike,
                                  double forward) {
            return surface.getVolatility(to_ql_date(to), strike, forward);
        });

    py::class_<sjd::VolatilitySurfaceMoneynessCMA,
               sjd::VolatilityRateSource,
               std::shared_ptr<sjd::VolatilitySurfaceMoneynessCMA>>(
        m, "VolatilitySurfaceMoneynessCMA")
        .def(py::init([](const py::object& anchor,
                         const py::iterable& observation_dates,
                         const std::vector<double>& moneyness,
                         const std::vector<std::vector<double>>& volatility,
                         SurfaceInterpolatorType interpolator,
                         bool extrapolate) {
                 return std::shared_ptr<sjd::VolatilitySurfaceMoneynessCMA>(
                     new sjd::VolatilitySurfaceMoneynessCMA(
                         to_ql_date(anchor),
                         to_ql_dates(observation_dates),
                         moneyness,
                         volatility,
                         interpolator,
                         extrapolate));
             }),
             py::arg("anchor_date"),
             py::arg("observation_dates"),
             py::arg("moneyness"),
             py::arg("volatility"),
             py::arg("interpolator"),
             py::arg("extrapolate") = true,
             "Row of volatility is moneyness. Column is an observation date. "
             "Moneyness outside the grid reads the nearest wing; extrapolate only extends time.")
        .def("is_ok", &sjd::VolatilitySurfaceMoneynessCMA::isOK)
        .def("error_messages", &sjd::VolatilitySurfaceMoneynessCMA::getErrorMessagesAsString)
        .def("get_volatility", [](const sjd::VolatilitySurfaceMoneynessCMA& surface,
                                  const py::object& to,
                                  double strike,
                                  double forward) {
            return surface.getVolatility(to_ql_date(to), strike, forward);
        });

    py::class_<sjd::RatesEnvironment, std::shared_ptr<sjd::RatesEnvironment>>(m, "RatesEnvironment")
        .def(py::init([](const py::object& anchor) {
                 return std::shared_ptr<sjd::RatesEnvironment>(
                     new sjd::RatesEnvironment(to_ql_date(anchor)));
             }),
             py::arg("anchor_date"))
        .def("is_ok", &sjd::RatesEnvironment::isOK)
        .def("error_messages", &sjd::RatesEnvironment::getErrorMessagesAsString)
        .def("validation_report", &sjd::RatesEnvironment::validationReport)
        .def("validate_report", &sjd::RatesEnvironment::validateReport)
        .def("anchor_date", [](const sjd::RatesEnvironment& re) {
            return from_ql_date(re.getAnchorDate());
        })
        .def("set_frs", [](sjd::RatesEnvironment& re,
                           std::shared_ptr<sjd::ForwardRateSource> curve) {
            re.setFRS(curve);
        })
        .def("set_drs", [](sjd::RatesEnvironment& re,
                           std::shared_ptr<sjd::DiscountCurve> curve) {
            re.setDRS(curve);
        })
        .def("set_vrs", [](sjd::RatesEnvironment& re,
                           std::shared_ptr<sjd::VolatilityRateSource> volatility) {
            re.setVRS(volatility);
        })
        .def("set_fixing_rs", [](sjd::RatesEnvironment& re,
                                 std::shared_ptr<sjd::DailyFixingCurve> curve) {
            re.setFixingRS(curve);
        });

    py::class_<sjd::Contract, std::shared_ptr<sjd::Contract>>(
        m, "Contract", "Base of every contract and portfolio. Not constructed directly.")
        .def("is_ok", &sjd::Contract::isOK)
        .def("error_messages", &sjd::Contract::getErrorMessagesAsString)
        .def("validation_report", &sjd::Contract::validationReport)
        .def("validate_report", [](sjd::Contract& contract,
                                   std::shared_ptr<sjd::RatesEnvironment> re) {
            return re ? contract.validateReport(re) : contract.validateReport();
        }, py::arg("rates_environment") = py::none(),
           "Validate, against the environment when one is given, and return the report.")
        .def("validate", [](sjd::Contract& contract,
                            std::shared_ptr<sjd::RatesEnvironment> re) {
            if (re)
                contract.validate(re);
            else
                contract.validate();
        }, py::arg("rates_environment") = py::none())
        .def("get_value", [](sjd::Contract& contract,
                             std::shared_ptr<sjd::RatesEnvironment> re) {
            return contract.getValue(re);
        }, py::arg("rates_environment"));

    py::class_<sjd::SingleInstrumentContract, sjd::Contract,
               std::shared_ptr<sjd::SingleInstrumentContract>>(
        m, "SingleInstrumentContract",
        "One instrument. Any subclass can go into a Portfolio.")
        .def("get_payoff", &sjd::SingleInstrumentContract::getPayoff, py::arg("spot"))
        .def("maturity_date", [](const sjd::SingleInstrumentContract& contract) {
            return from_ql_date(contract.getMaturityDate());
        })
        .def("settlement_date", [](const sjd::SingleInstrumentContract& contract) {
            return from_ql_date(contract.getSettlementDate());
        })
        .def_property_readonly("volume", &sjd::SingleInstrumentContract::getVolume)
        .def_property_readonly("buy_sell", &sjd::SingleInstrumentContract::getBuySell);

    py::class_<sjd::EuropeanOption, sjd::SingleInstrumentContract,
               std::shared_ptr<sjd::EuropeanOption>>(m, "EuropeanOption")
        .def(py::init([](const py::object& maturity,
                         const py::object& settlement,
                         double strike,
                         sjd::PutCall type,
                         double volume,
                         const py::object& premium_payment_date,
                         double premium_amount,
                         sjd::BuySell buy_sell) {
                 return std::shared_ptr<sjd::EuropeanOption>(
                     new sjd::EuropeanOption(
                         to_ql_date(maturity),
                         to_ql_date(settlement),
                         strike,
                         type,
                         volume,
                         to_ql_date(premium_payment_date),
                         premium_amount,
                         buy_sell));
             }),
             py::arg("maturity"),
             py::arg("settlement"),
             py::arg("strike"),
             py::arg("type"),
             py::arg("volume"),
             py::arg("premium_payment_date"),
             py::arg("premium_amount"),
             py::arg("buy_sell"))
        .def("get_option_value", [](sjd::EuropeanOption& option,
                                    std::shared_ptr<sjd::RatesEnvironment> re) {
            return option.getOptionValue(re);
        }, py::arg("rates_environment"), "The option alone, without the premium cashflow.");

    py::class_<sjd::EuropeanForward, sjd::SingleInstrumentContract,
               std::shared_ptr<sjd::EuropeanForward>>(m, "EuropeanForward")
        .def(py::init([](const py::object& maturity,
                         const py::object& settlement,
                         double strike,
                         double volume,
                         sjd::BuySell buy_sell) {
                 return std::shared_ptr<sjd::EuropeanForward>(
                     new sjd::EuropeanForward(
                         to_ql_date(maturity),
                         to_ql_date(settlement),
                         strike,
                         volume,
                         buy_sell));
             }),
             py::arg("maturity"),
             py::arg("settlement"),
             py::arg("strike"),
             py::arg("volume"),
             py::arg("buy_sell"),
             "Cash-settled forward with separate rate-set and settlement dates.")
        .def(py::init([](const py::object& settlement,
                         double strike,
                         double volume,
                         sjd::BuySell buy_sell) {
                 return std::shared_ptr<sjd::EuropeanForward>(
                     new sjd::EuropeanForward(
                         to_ql_date(settlement),
                         strike,
                         volume,
                         buy_sell));
             }),
             py::arg("settlement"),
             py::arg("strike"),
             py::arg("volume"),
             py::arg("buy_sell"),
             "Physically settled forward whose maturity equals settlement.");

    py::class_<sjd::AsianForward, sjd::SingleInstrumentContract,
               std::shared_ptr<sjd::AsianForward>>(m, "AsianForward", py::multiple_inheritance())
        .def(py::init([](const py::iterable& averaging_dates,
                         const py::object& settlement,
                         double strike,
                         double volume,
                         sjd::BuySell buy_sell) {
                 const std::vector<QuantLib::Date> dates = to_ql_dates(averaging_dates);
                 return std::shared_ptr<sjd::AsianForward>(
                     new sjd::AsianForward(
                         std::set<QuantLib::Date>(dates.begin(), dates.end()),
                         to_ql_date(settlement),
                         strike,
                         volume,
                         buy_sell));
             }),
             py::arg("averaging_dates"),
             py::arg("settlement"),
             py::arg("strike"),
             py::arg("volume"),
             py::arg("buy_sell"),
             "Asian forward using caller-supplied, already-adjusted averaging dates.")
        .def("averaging_dates", [](const sjd::AsianForward& forward) {
            return py_dates(forward.getAveragingDates());
        });

    py::class_<sjd::AsianOption, sjd::SingleInstrumentContract,
               std::shared_ptr<sjd::AsianOption>>(m, "AsianOption", py::multiple_inheritance())
        .def(py::init([](const py::iterable& averaging_dates,
                         const py::object& settlement,
                         double strike,
                         sjd::PutCall type,
                         double volume,
                         const py::object& premium_payment_date,
                         double premium_amount,
                         sjd::BuySell buy_sell) {
                 const std::vector<QuantLib::Date> dates = to_ql_dates(averaging_dates);
                 return std::shared_ptr<sjd::AsianOption>(
                     new sjd::AsianOption(
                         std::set<QuantLib::Date>(dates.begin(), dates.end()),
                         to_ql_date(settlement),
                         strike,
                         type,
                         volume,
                         to_ql_date(premium_payment_date),
                         premium_amount,
                         buy_sell));
             }),
             py::arg("averaging_dates"),
             py::arg("settlement"),
             py::arg("strike"),
             py::arg("type"),
             py::arg("volume"),
             py::arg("premium_payment_date"),
             py::arg("premium_amount"),
             py::arg("buy_sell"))
        .def("averaging_dates", [](const sjd::AsianOption& option) {
            return py_dates(option.getAveragingDates());
        })
        .def("get_option_value", [](sjd::AsianOption& option,
                                    std::shared_ptr<sjd::RatesEnvironment> re) {
            return option.getOptionValue(re);
        }, py::arg("rates_environment"), "The option alone, without the premium cashflow.");

    py::class_<sjd::RegCapUnderlyingFactors, std::shared_ptr<sjd::RegCapUnderlyingFactors>>(
        m, "RegCapUnderlyingFactors");

    m.def("reg_cap_factors", [](const std::string& description) {
        std::shared_ptr<sjd::RegCapUnderlyingFactors> factors = sjd::generateRegCapFactors(description);
        if (!factors)
        {
            throw py::value_error(
                "reg_cap_factors description must be ir, fx, equity, precious, or commodity");
        }
        return factors;
    }, py::arg("description"));

    py::class_<sjd::RegCapCreditParameters, std::shared_ptr<sjd::RegCapCreditParameters>>(
        m, "RegCapCreditParameters")
        .def(py::init<double, double>(),
             py::arg("counterparty_weighting"),
             py::arg("capital_charge"))
        .def("counterparty_weighting", &sjd::RegCapCreditParameters::getCounterpartyWeighting)
        .def("capital_charge", &sjd::RegCapCreditParameters::getCapitalCharge);

    py::class_<sjd::RegulatoryCapital, std::shared_ptr<sjd::RegulatoryCapital>>(
        m, "RegulatoryCapital")
        .def("is_ok", &sjd::RegulatoryCapital::isOK)
        .def("validation_report", &sjd::RegulatoryCapital::validationReport)
        .def("get_regulatory_capital", [](sjd::RegulatoryCapital& capital,
                                          std::shared_ptr<sjd::RatesEnvironment> environment) {
            return capital.getRegulatoryCapital(environment);
        }, py::arg("rates_environment"));

    py::class_<sjd::Margin, std::shared_ptr<sjd::Margin>>(m, "Margin")
        .def("is_ok", &sjd::Margin::isOK)
        .def("get_margin_balance", &sjd::Margin::getMarginBalance)
        .def("set_margin_balance", &sjd::Margin::setMarginBalance, py::arg("balance"));

    py::class_<sjd::SimpleMargin, sjd::Margin, std::shared_ptr<sjd::SimpleMargin>>(m, "SimpleMargin")
        .def(py::init([](const py::object& anchor,
                         bool use_my_threshold,
                         double my_threshold,
                         bool use_cpty_threshold,
                         double cpty_threshold,
                         double minimum_transfer_amount,
                         double rounding) {
                 return std::shared_ptr<sjd::SimpleMargin>(
                     new sjd::SimpleMargin(
                         to_ql_date(anchor),
                         use_my_threshold,
                         my_threshold,
                         use_cpty_threshold,
                         cpty_threshold,
                         minimum_transfer_amount,
                         rounding));
             }),
             py::arg("anchor_date"),
             py::arg("use_my_threshold"),
             py::arg("my_threshold"),
             py::arg("use_cpty_threshold"),
             py::arg("cpty_threshold"),
             py::arg("minimum_transfer_amount"),
             py::arg("rounding"),
             "Variation-margin CSA. Initial margin is not part of this model.")
        .def("get_margin_call", &sjd::SimpleMargin::getMarginCall, py::arg("mtm"));

    py::class_<sjd::Portfolio, sjd::Contract, std::shared_ptr<sjd::Portfolio>>(m, "Portfolio")
        .def(py::init([](const py::list& contracts) {
                 return std::shared_ptr<sjd::Portfolio>(new sjd::Portfolio(contracts_from(contracts)));
             }),
             py::arg("contracts"))
        .def(py::init([](const py::list& contracts, std::shared_ptr<sjd::Margin> margin) {
                 return std::shared_ptr<sjd::Portfolio>(
                     new sjd::Portfolio(contracts_from(contracts), margin));
             }),
             py::arg("contracts"),
             py::arg("margin"))
        .def("contracts", [](sjd::Portfolio& portfolio) {
            return portfolio.getUnderlyingSingleInstrumentContracts();
        }, "The single instruments in the portfolio.")
        .def("is_margined", &sjd::Portfolio::isMargined)
        .def("get_margin_balance", &sjd::Portfolio::getMarginBalance)
        .def("set_margin_balance", &sjd::Portfolio::setMarginBalance, py::arg("balance"))
        .def("get_margin_call", [](sjd::Portfolio& portfolio,
                                   std::shared_ptr<sjd::RatesEnvironment> re) {
            return portfolio.getMarginCall(re);
        }, py::arg("rates_environment"));

    py::class_<sjd::PortfolioB2CEM, sjd::Portfolio, std::shared_ptr<sjd::PortfolioB2CEM>>(
        m, "PortfolioB2CEM")
        .def(py::init([](const py::list& contracts,
                         std::shared_ptr<sjd::RegCapUnderlyingFactors> factors,
                         std::shared_ptr<sjd::RegCapCreditParameters> credit,
                         bool optimise) {
                 return std::shared_ptr<sjd::PortfolioB2CEM>(
                     new sjd::PortfolioB2CEM(
                         contract_inputs_from(contracts), factors, credit, optimise));
             }),
             py::arg("contracts"),
             py::arg("factors"),
             py::arg("credit"),
             py::arg("optimise") = false)
        .def(py::init([](const py::list& contracts,
                         std::shared_ptr<sjd::RegCapUnderlyingFactors> factors,
                         std::shared_ptr<sjd::RegCapCreditParameters> credit,
                         bool optimise,
                         std::shared_ptr<sjd::Margin> margin) {
                 return std::shared_ptr<sjd::PortfolioB2CEM>(
                     new sjd::PortfolioB2CEM(
                         contract_inputs_from(contracts), factors, credit, optimise, margin));
             }),
             py::arg("contracts"),
             py::arg("factors"),
             py::arg("credit"),
             py::arg("optimise"),
             py::arg("margin"))
        .def("is_ok", &sjd::PortfolioB2CEM::isOK)
        .def("error_messages", &sjd::PortfolioB2CEM::getErrorMessagesAsString)
        .def("validation_report", &sjd::PortfolioB2CEM::validationReport)
        .def("validate_report", [](sjd::PortfolioB2CEM& portfolio,
                                   std::shared_ptr<sjd::RatesEnvironment> environment) {
            return portfolio.validateReport(environment);
        })
        .def("regulatory_capital", &sjd::PortfolioB2CEM::regulatoryCapital)
        .def("get_regulatory_capital", [](sjd::PortfolioB2CEM& portfolio,
                                          std::shared_ptr<sjd::RatesEnvironment> environment) {
            return portfolio.getRegulatoryCapital(environment);
        }, py::arg("rates_environment"));

    py::class_<sjd::SingleFactorEconomicScenarioGenerator,
               std::shared_ptr<sjd::SingleFactorEconomicScenarioGenerator>>(
        m, "SingleFactorEconomicScenarioGenerator")
        .def(py::init([](std::shared_ptr<sjd::RatesEnvironment> environment) {
                 return std::shared_ptr<sjd::SingleFactorEconomicScenarioGenerator>(
                     new sjd::SingleFactorEconomicScenarioGenerator(environment));
             }),
             py::arg("rates_environment"))
        .def("is_ok", &sjd::SingleFactorEconomicScenarioGenerator::isOK)
        .def("validation_report", &sjd::SingleFactorEconomicScenarioGenerator::validationReport)
        .def("set_random_seed", &sjd::SingleFactorEconomicScenarioGenerator::setRandomSeed,
             py::arg("seed"),
             "Reseed the generator. The next run_monte_carlo draws the same paths for the same seed.");

    py::class_<sjd::PortfolioEvolver, std::shared_ptr<sjd::PortfolioEvolver>>(m, "PortfolioEvolver")
        .def(py::init(&make_evolver),
             py::arg("portfolio"),
             py::arg("path_generator"),
             py::arg("simulation_dates"),
             py::arg("capital") = static_cast<std::shared_ptr<sjd::RegulatoryCapital>>(nullptr),
             py::kw_only(),
             py::arg("holding_period") = 0,
             py::arg("holding_period_calendar") = py::none(),
             "With a holding period, a margined portfolio also simulates the date "
             "holding_period business days before each simulation date. The \"with HP\" "
             "columns subtract the margin from that earlier date.")
        .def_property_readonly("simulation_dates", [](sjd::PortfolioEvolver& evolver) {
            return py_dates(evolver.getSimulationDates());
        }, "The requested dates plus any margin reset dates.")
        .def_property_readonly("exposure_dates", [](sjd::PortfolioEvolver& evolver) {
            return py_dates(evolver.getExposureDates());
        })
        .def("is_ok", &sjd::PortfolioEvolver::isOK)
        .def("error_messages", &sjd::PortfolioEvolver::getErrorMessagesAsString)
        .def("validation_report", &sjd::PortfolioEvolver::validationReport)
        .def("validate_report", &sjd::PortfolioEvolver::validateReport)
        .def("run_monte_carlo", &sjd::PortfolioEvolver::runMonteCarlo, py::arg("number_of_paths"))
        .def("get_spot_path_detail", [](sjd::PortfolioEvolver& evolver,
                                        const std::vector<double>& spot_path) {
            std::vector<QuantLib::Date> dates;
            std::vector<std::string> headings;
            std::vector<std::vector<double>> rows;
            evolver.getSpotPathDetail(spot_path, dates, headings, rows);
            return detail_table(dates, headings, rows);
        }, py::arg("spot_path"))
        .def("get_monte_carlo_detail", [](sjd::PortfolioEvolver& evolver) {
            std::vector<QuantLib::Date> dates;
            std::vector<std::string> headings;
            std::vector<std::vector<double>> rows;
            evolver.getMonteCarloDetail(dates, headings, rows);
            return detail_table(dates, headings, rows);
        });

    py::class_<Simulation>(m, "Simulation",
                           "Simulated paths. Arrays are numpy, indexed [path, step] "
                           "and by_trade [path, step, trade].")
        .def_readonly("dates", &Simulation::dates,
                      "Every simulated date, including margin reset dates.")
        .def_readonly("exposure_steps", &Simulation::exposure_steps,
                      "Step of each requested simulation date.")
        .def_readonly("margin_reset_steps", &Simulation::margin_reset_steps,
                      "Step whose margin balance applies at each exposure step. Equal to "
                      "exposure_steps without a holding period.")
        .def_readonly("spot", &Simulation::spot,
                      "Forward for each simulation date, read on that date.")
        .def_readonly("mtm", &Simulation::mtm, "Portfolio value.")
        .def_readonly("margin", &Simulation::margin,
                      "Margin balance, or None when the portfolio is not margined.")
        .def_readonly("by_trade", &Simulation::by_trade,
                      "Value of each underlying contract, or None unless by_trade was requested.")
        .def_readonly("paths", &Simulation::paths)
        .def_readonly("steps", &Simulation::steps)
        .def_readonly("trades", &Simulation::trades)
        .def_readonly("antithetic", &Simulation::antithetic,
                      "Path i and path i + paths // 2 use opposite normals.")
        .def("__repr__", [](const Simulation& simulation) {
            return "<Simulation paths=" + std::to_string(simulation.paths) +
                   " steps=" + std::to_string(simulation.steps) +
                   " trades=" + std::to_string(simulation.trades) + ">";
        });

    m.def("simulate",
          [](std::shared_ptr<sjd::Portfolio> portfolio,
             std::shared_ptr<sjd::RatesEnvironment> environment,
             const py::iterable& simulation_dates,
             unsigned long paths,
             const py::object& seed,
             bool antithetic,
             bool by_trade,
             size_t holding_period,
             const py::object& holding_period_calendar) {
              std::shared_ptr<sjd::SingleFactorEconomicScenarioGenerator> generator(
                  new sjd::SingleFactorEconomicScenarioGenerator(environment));
              if (!seed.is_none())
                  generator->setRandomSeed(seed.cast<long>());
              std::shared_ptr<sjd::PortfolioEvolver> evolver = make_evolver(
                  portfolio, generator, simulation_dates, nullptr, holding_period, holding_period_calendar);
              sjd::SimulatedPaths simulated;
              {
                  py::gil_scoped_release release;
                  simulated = evolver->simulatePaths(paths, antithetic, by_trade);
              }
              if (simulated.paths == 0)
                  throw py::value_error("Cannot simulate: " + evolver->getErrorMessagesAsString());
              return simulation_from(simulated);
          },
          py::arg("portfolio"),
          py::arg("rates_environment"),
          py::arg("simulation_dates"),
          py::arg("paths") = 10000,
          py::kw_only(),
          py::arg("seed") = py::none(),
          py::arg("antithetic") = true,
          py::arg("by_trade") = false,
          py::arg("holding_period") = 0,
          py::arg("holding_period_calendar") = py::none(),
          "Simulate the portfolio along single-factor paths and return every value.\n\n"
          "The first simulation date must be the anchor date, and the forward must have a "
          "stochastic roll. With antithetic paths an odd count is rounded up. A seed makes "
          "the paths repeatable. A holding period on a margined portfolio adds a margin "
          "reset date holding_period business days before each simulation date. Raises "
          "ValueError if the portfolio or market cannot be simulated.");
}
