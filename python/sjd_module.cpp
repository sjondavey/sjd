#include <pybind11/eval.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <mutex>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/version.hpp>
#include <ql/version.hpp>

#include "sjd_build_info.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <cstdio>
#include <cstdlib>
#endif

#include <memory>
#include <ql/time/date.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>

#include <sjd/contracts/AsianForwardContract.h>
#include <sjd/contracts/AsianOptionContract.h>
#include <sjd/contracts/EuropeanForwardContract.h>
#include <sjd/contracts/EuropeanOptionContract.h>
#include <sjd/contracts/Portfolio/CVA/cva.h>
#include <sjd/contracts/Portfolio/CapitalModel.h>
#include <sjd/contracts/Portfolio/Margin/Margin.h>
#include <sjd/contracts/Portfolio/Portfolio.h>
#include <sjd/contracts/Portfolio/PortfolioEvolver.h>
#include <sjd/maths/maths.h>
#include <sjd/pricing/Black76Formula.h>
#include <sjd/rates/Discount/DiscountCurves.h>
#include <sjd/rates/Fixing/FixingCurves.h>
#include <sjd/rates/Forward/ForwardCurveFactory.h>
#include <sjd/rates/PathGenerators/GbmPathGenerator.h>
#include <sjd/rates/RatesEnvironment.h>
#include <sjd/rates/Volatility/VolatilityCurve.h>
#include <sjd/rates/Volatility/VolatilityCurveCMA.h>
#include <sjd/rates/Volatility/VolatilitySurfaceDelta.h>
#include <sjd/rates/Volatility/VolatilitySurfaceMoneynessCMA.h>
#include <sjd/tools/Calendars.h>
#include <sjd/tools/Common.h>
#include <sjd/tools/Errors.h>
#include <sjd/tools/Log.h>

using namespace std;
using namespace QuantLib;

namespace py = pybind11;

namespace
{

#if defined(_MSC_VER) && defined(_DEBUG)
    void exit_on_invalid_parameter(
        const wchar_t* expression, const wchar_t*, const wchar_t* file, unsigned int line, uintptr_t)
    {
        std::fwprintf(stderr, L"sjd: debug CRT check failed: %ls (%ls:%u)\n", expression ? expression : L"?",
                      file ? file : L"?", line);
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
        char* value = nullptr;
        size_t length = 0;
        _dupenv_s(&value, &length, "SJD_NO_DEBUG_DIALOGS");
        const bool quiet = value != nullptr;
        std::free(value);
        if (!quiet)
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
        return py::module_::import("datetime")
            .attr("date")(static_cast<int>(date.year()), static_cast<int>(date.month()), date.dayOfMonth());
    }

    QuantLib::DayCounter actual365()
    {
        return QuantLib::Actual365Fixed();
    }

    std::vector<std::shared_ptr<sjd::SingleInstrumentContract>> contracts_from(const py::list& contracts)
    {
        std::vector<std::shared_ptr<sjd::SingleInstrumentContract>> inputs;
        for (const py::handle& item : contracts)
        {
            if (!py::isinstance<sjd::SingleInstrumentContract>(item))
            {
                throw py::type_error("Portfolio contracts must be single instruments, such as EuropeanOption "
                                     "or AsianForward, not " +
                                     std::string(py::str(py::type::of(item).attr("__name__"))));
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

    struct ResolvedBook
    {
        std::shared_ptr<sjd::Portfolio> portfolio;
        std::vector<std::shared_ptr<sjd::Contract>> nettingSets;
    };

    ResolvedBook resolveBook(const py::object& book)
    {
        if (py::isinstance<sjd::Portfolio>(book))
            return {book.cast<std::shared_ptr<sjd::Portfolio>>(), {}};
        if (py::hasattr(book, "portfolio") && py::hasattr(book, "capital_model"))
        {
            std::shared_ptr<sjd::Portfolio> portfolio =
                book.attr("portfolio").cast<std::shared_ptr<sjd::Portfolio>>();
            std::shared_ptr<sjd::CurrentExposureMethod> model =
                book.attr("capital_model").cast<std::shared_ptr<sjd::CurrentExposureMethod>>();
            return {portfolio, model->nettingSetContracts()};
        }
        throw py::type_error("expected a Portfolio");
    }

    bool nettingSetsRequested(bool by_netting_set, const py::object& by_structure)
    {
        if (!by_structure.is_none())
        {
            if (by_netting_set)
                throw py::value_error("pass only one of by_netting_set and by_structure");
            return by_structure.cast<bool>();
        }
        return by_netting_set;
    }

    std::shared_ptr<sjd::PortfolioEvolver> make_evolver(
        const py::object& book,
        std::shared_ptr<sjd::SingleFactorEconomicScenarioGenerator> generator,
        const py::iterable& simulation_dates,
        size_t holding_period,
        const py::object& holding_period_calendar)
    {
        const ResolvedBook resolved = resolveBook(book);
        const std::set<QuantLib::Date> dates = simulation_dates_from(simulation_dates);
        std::shared_ptr<sjd::PortfolioEvolver> evolver;
        if (holding_period == 0)
        {
            evolver = std::make_shared<sjd::PortfolioEvolver>(resolved.portfolio, generator, dates);
        }
        else
        {
            if (holding_period_calendar.is_none())
                throw py::value_error("holding_period needs a holding_period_calendar");
            const QuantLib::Calendar calendar = holding_period_calendar.cast<const NamedCalendar&>().calendar;
            evolver = std::make_shared<sjd::PortfolioEvolver>(resolved.portfolio, generator, dates, calendar,
                                                              holding_period);
        }
        evolver->setNettingSets(resolved.nettingSets);
        return evolver;
    }

    struct Simulation
    {
        py::list dates;
        py::array_t<py::ssize_t> exposure_steps;
        py::array_t<py::ssize_t> margin_reset_steps;
        py::object spot = py::none();
        py::object mtm = py::none();
        py::object margin = py::none();
        py::object by_trade = py::none();
        py::object by_structure = py::none();
        py::object ee = py::none();
        py::object pfe = py::none();
        py::object mean_capital = py::none();
        bool aggregated = false;
        size_t paths = 0;
        size_t steps = 0;
        size_t trades = 0;
        size_t structures = 0;
        bool antithetic = false;
        // None when the paths were marked to supplied spots rather than drawn.
        py::object seed = py::none();
        py::dict metadata;
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

    py::object as_array(const std::vector<double>& values,
                        const std::vector<py::ssize_t>& shape,
                        bool float32)
    {
        if (float32)
        {
            py::array_t<float> array(shape);
            float* out = array.mutable_data();
            for (size_t i = 0; i < values.size(); ++i)
                out[i] = static_cast<float>(values[i]);
            return array;
        }
        return to_array(values, shape);
    }

    Simulation simulation_from(const sjd::SimulatedPaths& simulated, bool float32)
    {
        Simulation result;
        for (const QuantLib::Date& date : simulated.dates)
            result.dates.append(from_ql_date(date));
        result.exposure_steps = to_index_array(simulated.exposureSteps);
        result.margin_reset_steps = to_index_array(simulated.marginResetSteps);
        const py::ssize_t paths = static_cast<py::ssize_t>(simulated.paths);
        const py::ssize_t steps = static_cast<py::ssize_t>(simulated.steps);
        result.aggregated = simulated.aggregated;
        if (simulated.aggregated)
        {
            result.spot = as_array(simulated.meanSpot, {steps}, float32);
            result.mtm = as_array(simulated.meanMtm, {steps}, float32);
            result.margin =
                simulated.meanMargin.empty() ? py::none() : as_array(simulated.meanMargin, {steps}, float32);
            result.ee = as_array(simulated.expectedExposure, {steps}, float32);
            result.pfe = as_array(simulated.potentialFutureExposure, {steps}, float32);
            result.mean_capital =
                simulated.meanCapital.empty()
                    ? py::none()
                    : as_array(simulated.meanCapital,
                               {static_cast<py::ssize_t>(simulated.meanCapital.size())}, float32);
        }
        else
        {
            result.spot = as_array(simulated.spot, {paths, steps}, float32);
            result.mtm = as_array(simulated.mtm, {paths, steps}, float32);
            result.margin =
                simulated.margin.empty() ? py::none() : as_array(simulated.margin, {paths, steps}, float32);
            result.by_trade =
                simulated.byTrade.empty()
                    ? py::none()
                    : as_array(simulated.byTrade, {paths, steps, static_cast<py::ssize_t>(simulated.trades)},
                               float32);
            result.by_structure =
                simulated.byStructure.empty()
                    ? py::none()
                    : as_array(simulated.byStructure,
                               {paths, steps, static_cast<py::ssize_t>(simulated.structures)}, float32);
        }
        result.paths = simulated.paths;
        result.steps = simulated.steps;
        result.trades = simulated.trades;
        result.structures = simulated.structures;
        result.antithetic = simulated.antithetic;
        return result;
    }

    long long elapsedMilliseconds(std::chrono::steady_clock::time_point started)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                     started)
            .count();
    }

    std::string boostVersionText()
    {
        const int version = BOOST_VERSION;
        return std::to_string(version / 100000) + "." + std::to_string((version / 100) % 1000) + "." +
               std::to_string(version % 100);
    }

    py::dict libraryBuildInfo()
    {
        py::dict info;
        info["version"] = SJD_VERSION;
        info["compiler"] = SJD_COMPILER;
        info["build_type"] = SJD_BUILD_TYPE;
        info["quantlib"] = QL_VERSION;
        info["boost"] = boostVersionText();
        info["git_commit"] = SJD_GIT_COMMIT;
        info["git_dirty"] = SJD_GIT_DIRTY != 0;
        return info;
    }

    void appendWarningLines(std::vector<std::string>& lines, const sjd::ValidationReport& report)
    {
        const std::string text = report.warnings().toString();
        std::istringstream stream(text);
        std::string line;
        while (std::getline(stream, line))
        {
            if (!line.empty() && std::find(lines.begin(), lines.end(), line) == lines.end())
                lines.push_back(line);
        }
    }

    void appendSourceWarnings(std::vector<std::string>& lines,
                              const std::shared_ptr<sjd::RatesEnvironment>& environment)
    {
        if (!environment)
            return;
        appendWarningLines(lines, environment->validationReport());
        if (environment->hasDRS())
            appendWarningLines(lines, environment->getDRS()->validationReport());
        if (environment->hasForeignDRS())
            appendWarningLines(lines, environment->getForeignDRS()->validationReport());
        if (environment->hasFRS())
            appendWarningLines(lines, environment->getFRS()->validationReport());
        if (environment->hasVRS())
            appendWarningLines(lines, environment->getVRS()->validationReport());
        if (environment->hasFixingRS())
            appendWarningLines(lines, environment->getFixingRS()->validationReport());
    }

    py::dict runMetadata(const sjd::PortfolioEvolver& evolver,
                         const std::shared_ptr<sjd::Portfolio>& portfolio,
                         const std::shared_ptr<sjd::SingleFactorEconomicScenarioGenerator>& generator,
                         const std::shared_ptr<sjd::RatesEnvironment>& environment,
                         const py::dict& inputs,
                         long long setupMs,
                         long long pathsMs)
    {
        std::vector<std::string> lines;
        appendWarningLines(lines, evolver.validationReport());
        if (portfolio)
            appendWarningLines(lines, portfolio->validationReport());
        if (generator)
            appendWarningLines(lines, generator->validationReport());
        appendSourceWarnings(lines, environment);
        py::list warnings;
        for (const std::string& line : lines)
            warnings.append(line);
        py::dict timings;
        timings["setup_ms"] = setupMs;
        timings["paths_ms"] = pathsMs;
        py::dict metadata = libraryBuildInfo();
        metadata["timings"] = timings;
        metadata["warnings"] = warnings;
        metadata["inputs"] = inputs;
        return metadata;
    }

    void require_structures(bool by_netting_set, const sjd::SimulatedPaths& simulated)
    {
        if (by_netting_set && simulated.structures == 0)
            throw py::value_error("by_netting_set needs netting sets from a capital model");
    }

    std::mutex& pendingLogMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    std::vector<std::pair<sjd::log::Level, std::string>>& pendingLogs()
    {
        static std::vector<std::pair<sjd::log::Level, std::string>> pending;
        return pending;
    }

    int pythonLogLevel(sjd::log::Level level)
    {
        switch (level)
        {
        case sjd::log::Level::Trace:
            return 5;
        case sjd::log::Level::Debug:
            return 10;
        case sjd::log::Level::Info:
            return 20;
        case sjd::log::Level::Warn:
            return 30;
        case sjd::log::Level::Error:
            return 40;
        case sjd::log::Level::Off:
            return 0;
        }
        return 20;
    }

    void emitPythonLog(sjd::log::Level level, const std::string& message)
    {
        py::module_::import("logging")
            .attr("getLogger")("sjd._core")
            .attr("log")(pythonLogLevel(level), message);
    }

    void handleLogRecord(sjd::log::Level level, const std::string& message)
    {
        if (PyGILState_Check())
        {
            emitPythonLog(level, message);
            return;
        }
        std::lock_guard<std::mutex> lock(pendingLogMutex());
        pendingLogs().emplace_back(level, message);
    }

    void flushPythonLogs()
    {
        std::vector<std::pair<sjd::log::Level, std::string>> local;
        {
            std::lock_guard<std::mutex> lock(pendingLogMutex());
            local.swap(pendingLogs());
        }
        for (const auto& record : local)
            emitPythonLog(record.first, record.second);
    }

    struct FlushLogs
    {
        ~FlushLogs()
        {
            try
            {
                flushPythonLogs();
            }
            catch (...)
            {
            }
        }
    };

    sjd::log::Level levelFromName(const std::string& name)
    {
        std::string upper = name;
        for (char& character : upper)
            character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
        if (upper == "TRACE")
            return sjd::log::Level::Trace;
        if (upper == "DEBUG")
            return sjd::log::Level::Debug;
        if (upper == "INFO")
            return sjd::log::Level::Info;
        if (upper == "WARN" || upper == "WARNING")
            return sjd::log::Level::Warn;
        if (upper == "ERROR")
            return sjd::log::Level::Error;
        if (upper == "OFF")
            return sjd::log::Level::Off;
        throw py::value_error("Unknown log level " + name);
    }

    void throwIfNoPaths(const sjd::ValidationReport& report)
    {
        if (!report.isOK())
        {
            SJD_LOG_DEBUG(report.toString());
            throw sjd::ValidationError(report.toString(), report);
        }
        const sjd::ValidationReport failed =
            sjd::ValidationReport::failure("SIMULATION_EMPTY", "The simulation produced no paths");
        throw sjd::ValidationError(failed.toString(), failed);
    }

    std::string severityLabel(sjd::ValidationSeverity severity)
    {
        switch (severity)
        {
        case sjd::ValidationSeverity::INFO:
            return "INFO";
        case sjd::ValidationSeverity::WARNING:
            return "WARNING";
        case sjd::ValidationSeverity::ERROR:
            return "ERROR";
        }
        return "ERROR";
    }

    std::string htmlEscape(const std::string& text)
    {
        std::string escaped;
        escaped.reserve(text.size());
        for (char character : text)
        {
            switch (character)
            {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            default:
                escaped += character;
                break;
            }
        }
        return escaped;
    }

    std::string joinedPath(const std::vector<std::string>& path)
    {
        std::string joined;
        for (size_t i = 0; i < path.size(); ++i)
        {
            if (i > 0)
                joined += '/';
            joined += path[i];
        }
        return joined;
    }

    std::string issueLine(const sjd::ValidationIssue& issue)
    {
        return joinedPath(issue.path) + ": " + issue.code + ": " + issue.message;
    }

    py::dict issueDict(const sjd::ValidationIssue& issue)
    {
        py::dict result;
        result["code"] = issue.code;
        result["message"] = issue.message;
        result["severity"] = severityLabel(issue.severity);
        py::list path;
        for (const std::string& element : issue.path)
            path.append(element);
        result["path"] = path;
        return result;
    }

    std::string reportHtml(const sjd::ValidationReport& report)
    {
        std::ostringstream html;
        html << "<table><thead><tr><th>severity</th><th>path</th><th>code</th><th>message</th></tr></"
                "thead><tbody>";
        for (const sjd::ValidationIssue& issue : report.getIssues())
        {
            html << "<tr><td>" << severityLabel(issue.severity) << "</td><td>"
                 << htmlEscape(joinedPath(issue.path)) << "</td><td>" << htmlEscape(issue.code) << "</td><td>"
                 << htmlEscape(issue.message) << "</td></tr>";
        }
        html << "</tbody></table>";
        return html.str();
    }

} // namespace

// is_ok on a Validatable used to re-run its checks. Keep that behaviour for the
// deprecated Python alias. Contract::is_ok stays a pure read of the last report,
// so validate(environment) is still what is_ok reports.
template <typename T> bool isOkByValidating(T& object)
{
    return object.validate().isOK();
}

template <typename T>
sjd::ValidationReport validateOptionalEnvironment(T& object,
                                                  std::shared_ptr<sjd::RatesEnvironment> environment)
{
    if (environment)
        return object.validate(environment);
    return object.validate();
}

// validate() returns the report. is_valid and report only read the last one.
// is_ok, error_messages, validation_report and validate_report stay as the old names.
template <typename T, typename... Options>
py::class_<T, Options...>& bindValidation(py::class_<T, Options...>& cls, bool isOkReruns = true)
{
    cls.def_property_readonly("is_valid", &T::isValid, "True when the last report has no errors.")
        .def_property_readonly("report", &T::validationReport,
                               "The last validation report. Does not run the checks.");
    if (isOkReruns)
    {
        cls.def("is_ok", &isOkByValidating<T>, "Deprecated. Calls validate() and reports whether it passed.");
    }
    else
    {
        cls.def("is_ok", &T::isOK, "Deprecated. Reads the last report and does not run the checks.");
    }
    cls.def("error_messages", &T::getErrorMessagesAsString,
            "Deprecated. The old class name and message text.")
        .def("validation_report", &T::validationReport, "Deprecated alias of report.")
        .def(
            "validate", [](T& object) { return object.validate(); }, "Run the checks and return the report.")
        .def(
            "validate_report", [](T& object) { return object.validate(); },
            "Deprecated alias of validate().");
    return cls;
}

// Contracts and regulatory capital check against a rates environment when one is given.
// is_ok stays a pure read: it used to report the last validate(environment), not a fresh one.
template <typename T, typename... Options>
py::class_<T, Options...>& bindEnvironmentValidation(py::class_<T, Options...>& cls)
{
    cls.def_property_readonly("is_valid", &T::isValid, "True when the last report has no errors.")
        .def_property_readonly("report", &T::validationReport,
                               "The last validation report. Does not run the checks.")
        .def("validate", &validateOptionalEnvironment<T>, py::arg("rates_environment") = py::none(),
             "Validate, against the environment when one is given, and return the report.")
        .def("is_ok", &T::isOK, "Deprecated. Reads the last report and does not run the checks.")
        .def("error_messages", &T::getErrorMessagesAsString,
             "Deprecated. The old class name and message text.")
        .def("validation_report", &T::validationReport, "Deprecated alias of report.")
        .def("validate_report", &validateOptionalEnvironment<T>, py::arg("rates_environment") = py::none(),
             "Deprecated alias of validate().");
    return cls;
}

void registerTranslatedErrors(py::module_& module)
{
    py::exec(R"py(
class SjdError(RuntimeError):
    """A failure from the pricing library. code is the stable identifier."""

    def __init__(self, message, code):
        super().__init__(message)
        self.code = code


class ConfigurationError(SjdError, ValueError):
    """An unknown name or an option the type cannot accept."""

    def __init__(self, message):
        super().__init__(message, "CONFIGURATION")


class NumericalError(SjdError, ArithmeticError):
    """A solver did not converge, or a strike and a delta could not be converted."""

    def __init__(self, message):
        super().__init__(message, "NUMERICAL")


class ValidationError(SjdError, ValueError):
    """A calculation was refused because validation failed. report holds the issues."""

    def __init__(self, message, report):
        super().__init__(message, "VALIDATION")
        self.report = report
)py",
             module.attr("__dict__"));

    py::register_exception_translator(
        [](std::exception_ptr pointer)
        {
            try
            {
                if (pointer)
                    std::rethrow_exception(pointer);
            }
            catch (const sjd::ValidationError& error)
            {
                py::object type = py::module_::import("sjd._core").attr("ValidationError");
                py::object instance = type(std::string(error.what()), py::cast(error.report()));
                PyErr_SetObject(type.ptr(), instance.ptr());
            }
            catch (const sjd::ConfigurationError& error)
            {
                py::object type = py::module_::import("sjd._core").attr("ConfigurationError");
                py::object instance = type(std::string(error.what()));
                PyErr_SetObject(type.ptr(), instance.ptr());
            }
            catch (const sjd::NumericalError& error)
            {
                py::object type = py::module_::import("sjd._core").attr("NumericalError");
                py::object instance = type(std::string(error.what()));
                PyErr_SetObject(type.ptr(), instance.ptr());
            }
            catch (const sjd::Error& error)
            {
                py::object type = py::module_::import("sjd._core").attr("SjdError");
                py::object instance = type(std::string(error.what()), error.code());
                PyErr_SetObject(type.ptr(), instance.ptr());
            }
        });
}

PYBIND11_MODULE(_core, m)
{
    report_debug_checks_to_stderr();
    sjd::log::setRecordHandler(&handleLogRecord);
    py::module_::import("logging").attr("addLevelName")(5, "TRACE");
    m.doc() = "sjd derivative pricing (compiled core; import sjd rather than sjd._core)";

    py::enum_<sjd::BuySell>(m, "BuySell").value("BUY", sjd::BuySell::BUY).value("SELL", sjd::BuySell::SELL);

    py::enum_<sjd::PutCall>(m, "PutCall").value("PUT", sjd::PutCall::PUT).value("CALL", sjd::PutCall::CALL);

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
        .def(py::init(
                 [](const std::string& name, const std::vector<std::string>& components)
                 {
                     NamedCalendar result;
                     result.name = name;
                     result.components = components;
                     result.calendar =
                         components.empty() ? sjd::calendarNamed(name) : sjd::jointCalendar(components);
                     return result;
                 }),
             py::arg("name"), py::arg("components") = std::vector<std::string>())
        .def_readonly("name", &NamedCalendar::name)
        .def_property_readonly("components", [](const NamedCalendar& calendar)
                               { return py::tuple(py::cast(calendar.components)); })
        .def_static("names", &sjd::calendarNames, "Calendar names accepted by Calendar().")
        .def(
            "is_business_day", [](const NamedCalendar& calendar, const py::object& day)
            { return calendar.calendar.isBusinessDay(to_ql_date(day)); }, py::arg("day"))
        .def(
            "is_holiday", [](const NamedCalendar& calendar, const py::object& day)
            { return calendar.calendar.isHoliday(to_ql_date(day)); }, py::arg("day"),
            "True on weekends as well as public holidays.")
        .def(
            "adjust",
            [](const NamedCalendar& calendar, const py::object& day,
               QuantLib::BusinessDayConvention convention)
            { return from_ql_date(calendar.calendar.adjust(to_ql_date(day), convention)); },
            py::arg("day"), py::arg("convention") = QuantLib::Following)
        .def(
            "advance",
            [](const NamedCalendar& calendar, const py::object& day, int count, QuantLib::TimeUnit unit,
               QuantLib::BusinessDayConvention convention, bool end_of_month) {
                return from_ql_date(
                    calendar.calendar.advance(to_ql_date(day), count, unit, convention, end_of_month));
            },
            py::arg("day"), py::arg("count"), py::arg("unit") = QuantLib::Days,
            py::arg("convention") = QuantLib::Following, py::arg("end_of_month") = false,
            "Move count business days with unit DAYS. Other units move the calendar "
            "period and then adjust by convention.")
        .def(
            "business_days_between",
            [](const NamedCalendar& calendar, const py::object& start, const py::object& end,
               bool include_first, bool include_last)
            {
                return calendar.calendar.businessDaysBetween(to_ql_date(start), to_ql_date(end),
                                                             include_first, include_last);
            },
            py::arg("start"), py::arg("end"), py::arg("include_first") = true,
            py::arg("include_last") = false)
        .def(
            "holidays",
            [](const NamedCalendar& calendar, const py::object& start, const py::object& end,
               bool include_weekends) {
                return py_dates(
                    calendar.calendar.holidayList(to_ql_date(start), to_ql_date(end), include_weekends));
            },
            py::arg("start"), py::arg("end"), py::arg("include_weekends") = false)
        .def("__repr__", [](const NamedCalendar& calendar) { return "<Calendar " + calendar.name + ">"; });

    py::enum_<sjd::ValidationSeverity>(m, "ValidationSeverity")
        .value("INFO", sjd::ValidationSeverity::INFO)
        .value("WARNING", sjd::ValidationSeverity::WARNING)
        .value("ERROR", sjd::ValidationSeverity::ERROR);

    py::class_<sjd::ValidationIssue>(m, "ValidationIssue")
        .def_readonly("code", &sjd::ValidationIssue::code)
        .def_readonly("message", &sjd::ValidationIssue::message)
        .def_readonly("severity", &sjd::ValidationIssue::severity)
        .def_property_readonly("path",
                               [](const sjd::ValidationIssue& issue)
                               {
                                   py::tuple path(issue.path.size());
                                   for (size_t i = 0; i < issue.path.size(); ++i)
                                       path[i] = issue.path[i];
                                   return path;
                               })
        .def("__str__", [](const sjd::ValidationIssue& issue) { return issueLine(issue); })
        .def("__repr__", [](const sjd::ValidationIssue& issue)
             { return "<ValidationIssue " + severityLabel(issue.severity) + " " + issue.code + ">"; })
        .def("to_dict", [](const sjd::ValidationIssue& issue) { return issueDict(issue); });

    py::class_<sjd::ValidationReport>(m, "ValidationReport")
        .def_property_readonly("ok", &sjd::ValidationReport::isOK)
        .def_property_readonly("issues",
                               [](const sjd::ValidationReport& report)
                               {
                                   const std::vector<sjd::ValidationIssue>& source = report.getIssues();
                                   py::tuple issues(source.size());
                                   for (size_t i = 0; i < source.size(); ++i)
                                       issues[i] = py::cast(source[i]);
                                   return issues;
                               })
        .def("__str__", &sjd::ValidationReport::toString)
        .def("__repr__",
             [](const sjd::ValidationReport& report)
             {
                 return "<ValidationReport ok=" + std::string(report.isOK() ? "True" : "False") +
                        " issues=" + std::to_string(report.getIssues().size()) + ">";
             })
        .def("to_dict",
             [](const sjd::ValidationReport& report)
             {
                 py::dict result;
                 result["ok"] = report.isOK();
                 py::list issues;
                 for (const sjd::ValidationIssue& issue : report.getIssues())
                     issues.append(issueDict(issue));
                 result["issues"] = issues;
                 return result;
             })
        .def("_repr_html_", [](const sjd::ValidationReport& report) { return reportHtml(report); })
        .def(
            "raise_if_invalid",
            [](const sjd::ValidationReport& report)
            {
                if (!report.isOK())
                    throw sjd::ValidationError(report.toString(), report);
            },
            "Raise ValidationError when the report contains an error.");

    registerTranslatedErrors(m);

    py::class_<sjd::Black76Call>(m, "Black76Call")
        .def(py::init(
                 [](double forward, double strike, double standardDeviation, double discountFactor) {
                     return std::make_unique<sjd::Black76Call>(forward, strike, standardDeviation,
                                                               discountFactor);
                 }),
             py::arg("forward"), py::arg("strike"), py::arg("standard_deviation"), py::arg("discount_factor"))
        .def("premium", &sjd::Black76Call::getPremium)
        .def("delta", &sjd::Black76Call::getDelta);

    py::class_<sjd::Black76Put>(m, "Black76Put")
        .def(py::init(
                 [](double forward, double strike, double standardDeviation, double discountFactor) {
                     return std::make_unique<sjd::Black76Put>(forward, strike, standardDeviation,
                                                              discountFactor);
                 }),
             py::arg("forward"), py::arg("strike"), py::arg("standard_deviation"), py::arg("discount_factor"))
        .def("premium", &sjd::Black76Put::getPremium)
        .def("delta", &sjd::Black76Put::getDelta);

    py::enum_<sjd::ArrayInterpolatorType>(m, "CurveInterpolator")
        .value("LINEAR", sjd::ArrayInterpolatorType::LINEAR)
        .value("CUBIC", sjd::ArrayInterpolatorType::CUBIC);

    py::enum_<sjd::ForwardAveraging>(m, "ForwardAveraging")
        .value("POINT", sjd::ForwardAveraging::Point)
        .value("CALENDAR_MONTH", sjd::ForwardAveraging::CalendarMonth);

    py::enum_<sjd::ForwardRoll>(m, "ForwardRoll")
        .value("DETERMINISTIC", sjd::ForwardRoll::Deterministic)
        .value("SPOT_VOLATILITY", sjd::ForwardRoll::SpotVolatility)
        .value("FORWARD_VOLATILITY", sjd::ForwardRoll::ForwardVolatility);

    // Base for every volatility curve and surface, so any of them can be passed
    // where a volatility source is expected.
    auto volatilitySource = py::class_<sjd::VolatilityRateSource, std::shared_ptr<sjd::VolatilityRateSource>>(
        m, "VolatilityRateSource");
    bindValidation(volatilitySource);
    volatilitySource.def("anchor_date", [](const sjd::VolatilityRateSource& source)
                         { return from_ql_date(source.getAnchorDate()); });

    auto forwardCurve =
        py::class_<sjd::ForwardRateSource, std::shared_ptr<sjd::ForwardRateSource>>(m, "ForwardCurve");
    bindValidation(forwardCurve);
    forwardCurve
        .def(py::init(
                 [](const py::object& anchor, const py::iterable& observation_dates,
                    const std::vector<double>& forwards, bool allow_extrapolation,
                    sjd::ForwardAveraging averaging, sjd::ForwardRoll roll,
                    sjd::ArrayInterpolatorType interpolation,
                    std::shared_ptr<sjd::VolatilityRateSource> volatility)
                 {
                     return sjd::makeForwardCurve(to_ql_date(anchor), to_ql_dates(observation_dates),
                                                  forwards, interpolation, allow_extrapolation, averaging,
                                                  roll, volatility);
                 }),
             py::arg("anchor_date"), py::arg("observation_dates"), py::arg("forwards"),
             py::arg("allow_extrapolation") = false, py::kw_only(),
             py::arg("averaging") = sjd::ForwardAveraging::Point,
             py::arg("roll") = sjd::ForwardRoll::Deterministic,
             py::arg("interpolation") = sjd::ArrayInterpolatorType::LINEAR,
             py::arg("volatility") = py::none(),
             "Interpolated forward curve.\n\n"
             "averaging=CALENDAR_MONTH reads observation dates by month and year only.\n"
             "roll=DETERMINISTIC ignores volatility. SPOT_VOLATILITY and FORWARD_VOLATILITY "
             "need a volatility curve or surface, and PortfolioEvolver needs one of them. "
             "Rolled curves interpolate linearly whatever the interpolation here.")
        .def("anchor_date",
             [](const sjd::ForwardRateSource& curve) { return from_ql_date(curve.getAnchorDate()); })
        .def("final_date",
             [](const sjd::ForwardRateSource& curve) { return from_ql_date(curve.getFinalDate()); })
        .def("get_forward", [](const sjd::ForwardRateSource& curve, const py::object& to)
             { return curve.getForward(to_ql_date(to)); });

    auto discountCurve =
        py::class_<sjd::DiscountCurve, std::shared_ptr<sjd::DiscountCurve>>(m, "DiscountCurve");
    bindValidation(discountCurve);
    discountCurve
        .def(py::init(
                 [](const py::object& anchor, const py::iterable& observation_dates,
                    const std::vector<double>& discount_factors, bool allow_extrapolation)
                 {
                     return std::make_shared<sjd::DiscountCurve>(
                         to_ql_date(anchor), to_ql_dates(observation_dates), discount_factors,
                         sjd::ArrayInterpolatorType::LINEAR, allow_extrapolation);
                 }),
             py::arg("anchor_date"), py::arg("observation_dates"), py::arg("discount_factors"),
             py::arg("allow_extrapolation") = false)
        .def("get_discount_factor", [](const sjd::DiscountCurve& curve, const py::object& to)
             { return curve.getDiscountFactor(to_ql_date(to)); });

    py::class_<sjd::VolatilityCurve, sjd::VolatilityRateSource, std::shared_ptr<sjd::VolatilityCurve>>(
        m, "VolatilityCurve")
        .def(py::init(
                 [](const py::object& anchor, const py::iterable& observation_dates,
                    const std::vector<double>& volatilities, bool allow_extrapolation)
                 {
                     return std::make_shared<sjd::VolatilityCurve>(
                         to_ql_date(anchor), to_ql_dates(observation_dates), volatilities, actual365(),
                         sjd::ArrayInterpolatorType::LINEAR, allow_extrapolation);
                 }),
             py::arg("anchor_date"), py::arg("observation_dates"), py::arg("volatilities"),
             py::arg("allow_extrapolation") = false,
             "Volatilities are interpolated in standard-deviation space. "
             "Year fractions use Actual/365 Fixed (not a QuantLib-Python type).")
        .def("get_volatility", [](const sjd::VolatilityCurve& curve, const py::object& to)
             { return curve.getVolatility(to_ql_date(to)); });

    py::class_<sjd::VolatilityCurveCMA, sjd::VolatilityRateSource, std::shared_ptr<sjd::VolatilityCurveCMA>>(
        m, "VolatilityCurveCMA")
        .def(py::init(
                 [](const py::object& anchor, const py::iterable& observation_dates,
                    const std::vector<double>& volatilities, bool allow_extrapolation)
                 {
                     return std::make_shared<sjd::VolatilityCurveCMA>(
                         to_ql_date(anchor), to_ql_dates(observation_dates), volatilities, actual365(),
                         sjd::ArrayInterpolatorType::LINEAR, allow_extrapolation);
                 }),
             py::arg("anchor_date"), py::arg("observation_dates"), py::arg("volatilities"),
             py::arg("allow_extrapolation") = false,
             "Calendar-month-average volatility curve using Actual/365 Fixed.")
        .def("get_volatility", [](const sjd::VolatilityCurveCMA& curve, const py::object& to)
             { return curve.getVolatility(to_ql_date(to)); })
        .def("get_standard_deviation", [](const sjd::VolatilityCurveCMA& curve, const py::object& to)
             { return curve.getStandardDeviation(to_ql_date(to)); });

    auto fixingCurve =
        py::class_<sjd::DailyFixingCurve, std::shared_ptr<sjd::DailyFixingCurve>>(m, "DailyFixingCurve");
    bindValidation(fixingCurve);
    fixingCurve
        .def(py::init(
                 [](const py::iterable& fixing_dates, const std::vector<double>& fixing_rates) {
                     return std::make_shared<sjd::DailyFixingCurve>(to_ql_dates(fixing_dates), fixing_rates);
                 }),
             py::arg("fixing_dates"), py::arg("fixing_rates"))
        .def("get_rate", [](const sjd::DailyFixingCurve& curve, const py::object& on_date)
             { return curve.getRateSetOn(to_ql_date(on_date)); });

    py::enum_<SurfaceInterpolatorType>(m, "SurfaceInterpolator")
        .value("BILINEAR", BILINEAR)
        .value("BICUBIC", BICUBIC);

    py::class_<sjd::VolatilitySurfaceDelta, sjd::VolatilityRateSource,
               std::shared_ptr<sjd::VolatilitySurfaceDelta>>(m, "VolatilitySurfaceDelta")
        .def(py::init(
                 [](const py::object& anchor, const py::iterable& observation_dates,
                    const std::vector<double>& deltas, const std::vector<std::vector<double>>& volatility,
                    SurfaceInterpolatorType interpolator, bool extrapolate)
                 {
                     return std::make_shared<sjd::VolatilitySurfaceDelta>(
                         to_ql_date(anchor), to_ql_dates(observation_dates), deltas, volatility, interpolator,
                         extrapolate);
                 }),
             py::arg("anchor_date"), py::arg("observation_dates"), py::arg("deltas"), py::arg("volatility"),
             py::arg("interpolator"), py::arg("extrapolate") = false,
             "Row of volatility is a put delta in percent. Column is an observation date. "
             "Deltas outside the grid read the nearest wing; extrapolate only extends time.")
        .def("get_volatility",
             [](const sjd::VolatilitySurfaceDelta& surface, const py::object& to, double strike,
                double forward) { return surface.getVolatility(to_ql_date(to), strike, forward); })
        .def(
            "strike_from_delta",
            [](const sjd::VolatilitySurfaceDelta& surface, const py::object& day, double delta,
               double forward) { return surface.calculateStrikeFromDelta(to_ql_date(day), delta, forward); },
            py::arg("day"), py::arg("delta"), py::arg("forward"),
            "Strike whose put delta, in percent, matches delta. Raises NumericalError when the solve fails.");

    py::class_<sjd::VolatilitySurfaceMoneynessCMA, sjd::VolatilityRateSource,
               std::shared_ptr<sjd::VolatilitySurfaceMoneynessCMA>>(m, "VolatilitySurfaceMoneynessCMA")
        .def(py::init(
                 [](const py::object& anchor, const py::iterable& observation_dates,
                    const std::vector<double>& moneyness, const std::vector<std::vector<double>>& volatility,
                    SurfaceInterpolatorType interpolator, bool extrapolate)
                 {
                     return std::make_shared<sjd::VolatilitySurfaceMoneynessCMA>(
                         to_ql_date(anchor), to_ql_dates(observation_dates), moneyness, volatility,
                         interpolator, extrapolate);
                 }),
             py::arg("anchor_date"), py::arg("observation_dates"), py::arg("moneyness"),
             py::arg("volatility"), py::arg("interpolator"), py::arg("extrapolate") = true,
             "Row of volatility is moneyness. Column is an observation date. "
             "Moneyness outside the grid reads the nearest wing; extrapolate only extends time.")
        .def("get_volatility",
             [](const sjd::VolatilitySurfaceMoneynessCMA& surface, const py::object& to, double strike,
                double forward) { return surface.getVolatility(to_ql_date(to), strike, forward); });

    auto ratesEnvironment =
        py::class_<sjd::RatesEnvironment, std::shared_ptr<sjd::RatesEnvironment>>(m, "RatesEnvironment");
    bindValidation(ratesEnvironment);
    ratesEnvironment
        .def(py::init([](const py::object& anchor)
                      { return std::make_shared<sjd::RatesEnvironment>(to_ql_date(anchor)); }),
             py::arg("anchor_date"))
        .def("anchor_date", [](const sjd::RatesEnvironment& re) { return from_ql_date(re.getAnchorDate()); })
        .def("set_frs", [](sjd::RatesEnvironment& re, std::shared_ptr<sjd::ForwardRateSource> curve)
             { re.setFRS(curve); })
        .def("set_drs",
             [](sjd::RatesEnvironment& re, std::shared_ptr<sjd::DiscountCurve> curve) { re.setDRS(curve); })
        .def("set_vrs", [](sjd::RatesEnvironment& re, std::shared_ptr<sjd::VolatilityRateSource> volatility)
             { re.setVRS(volatility); })
        .def("set_fixing_rs", [](sjd::RatesEnvironment& re, std::shared_ptr<sjd::DailyFixingCurve> curve)
             { re.setFixingRS(curve); });

    auto contract = py::class_<sjd::Contract, std::shared_ptr<sjd::Contract>>(
        m, "Contract", "Base of every contract and portfolio. Not constructed directly.");
    bindEnvironmentValidation(contract);
    contract.def(
        "get_value", [](sjd::Contract& priced, std::shared_ptr<sjd::RatesEnvironment> re)
        { return priced.getValue(re); }, py::arg("rates_environment"));

    py::class_<sjd::SingleInstrumentContract, sjd::Contract, std::shared_ptr<sjd::SingleInstrumentContract>>(
        m, "SingleInstrumentContract", "One instrument. Any subclass can go into a Portfolio.")
        .def("get_payoff", &sjd::SingleInstrumentContract::getPayoff, py::arg("spot"))
        .def("maturity_date", [](const sjd::SingleInstrumentContract& contract)
             { return from_ql_date(contract.getMaturityDate()); })
        .def("settlement_date", [](const sjd::SingleInstrumentContract& contract)
             { return from_ql_date(contract.getSettlementDate()); })
        .def_property_readonly("volume", &sjd::SingleInstrumentContract::getVolume)
        .def_property_readonly("buy_sell", &sjd::SingleInstrumentContract::getBuySell);

    py::class_<sjd::EuropeanOption, sjd::SingleInstrumentContract, std::shared_ptr<sjd::EuropeanOption>>(
        m, "EuropeanOption")
        .def(py::init(
                 [](const py::object& maturity, const py::object& settlement, double strike,
                    sjd::PutCall type, double volume, const py::object& premium_payment_date,
                    double premium_amount, sjd::BuySell buy_sell)
                 {
                     return std::make_shared<sjd::EuropeanOption>(
                         to_ql_date(maturity), to_ql_date(settlement), strike, type, volume,
                         to_ql_date(premium_payment_date), premium_amount, buy_sell);
                 }),
             py::arg("maturity"), py::arg("settlement"), py::arg("strike"), py::arg("type"),
             py::arg("volume"), py::arg("premium_payment_date"), py::arg("premium_amount"),
             py::arg("buy_sell"))
        .def(
            "get_option_value", [](sjd::EuropeanOption& option, std::shared_ptr<sjd::RatesEnvironment> re)
            { return option.getOptionValue(re); }, py::arg("rates_environment"),
            "The option alone, without the premium cashflow.");

    py::class_<sjd::EuropeanForward, sjd::SingleInstrumentContract, std::shared_ptr<sjd::EuropeanForward>>(
        m, "EuropeanForward")
        .def(py::init(
                 [](const py::object& maturity, const py::object& settlement, double strike, double volume,
                    sjd::BuySell buy_sell)
                 {
                     return std::make_shared<sjd::EuropeanForward>(
                         to_ql_date(maturity), to_ql_date(settlement), strike, volume, buy_sell);
                 }),
             py::arg("maturity"), py::arg("settlement"), py::arg("strike"), py::arg("volume"),
             py::arg("buy_sell"), "Cash-settled forward with separate rate-set and settlement dates.")
        .def(py::init(
                 [](const py::object& settlement, double strike, double volume, sjd::BuySell buy_sell) {
                     return std::make_shared<sjd::EuropeanForward>(to_ql_date(settlement), strike, volume,
                                                                   buy_sell);
                 }),
             py::arg("settlement"), py::arg("strike"), py::arg("volume"), py::arg("buy_sell"),
             "Physically settled forward whose maturity equals settlement.");

    py::class_<sjd::AsianForward, sjd::SingleInstrumentContract, std::shared_ptr<sjd::AsianForward>>(
        m, "AsianForward", py::multiple_inheritance())
        .def(py::init(
                 [](const py::iterable& averaging_dates, const py::object& settlement, double strike,
                    double volume, sjd::BuySell buy_sell)
                 {
                     const std::vector<QuantLib::Date> dates = to_ql_dates(averaging_dates);
                     return std::make_shared<sjd::AsianForward>(
                         std::set<QuantLib::Date>(dates.begin(), dates.end()), to_ql_date(settlement), strike,
                         volume, buy_sell);
                 }),
             py::arg("averaging_dates"), py::arg("settlement"), py::arg("strike"), py::arg("volume"),
             py::arg("buy_sell"), "Asian forward using caller-supplied, already-adjusted averaging dates.")
        .def("averaging_dates",
             [](const sjd::AsianForward& forward) { return py_dates(forward.getAveragingDates()); });

    py::class_<sjd::AsianOption, sjd::SingleInstrumentContract, std::shared_ptr<sjd::AsianOption>>(
        m, "AsianOption", py::multiple_inheritance())
        .def(py::init(
                 [](const py::iterable& averaging_dates, const py::object& settlement, double strike,
                    sjd::PutCall type, double volume, const py::object& premium_payment_date,
                    double premium_amount, sjd::BuySell buy_sell)
                 {
                     const std::vector<QuantLib::Date> dates = to_ql_dates(averaging_dates);
                     return std::make_shared<sjd::AsianOption>(
                         std::set<QuantLib::Date>(dates.begin(), dates.end()), to_ql_date(settlement), strike,
                         type, volume, to_ql_date(premium_payment_date), premium_amount, buy_sell);
                 }),
             py::arg("averaging_dates"), py::arg("settlement"), py::arg("strike"), py::arg("type"),
             py::arg("volume"), py::arg("premium_payment_date"), py::arg("premium_amount"),
             py::arg("buy_sell"))
        .def("averaging_dates",
             [](const sjd::AsianOption& option) { return py_dates(option.getAveragingDates()); })
        .def(
            "get_option_value", [](sjd::AsianOption& option, std::shared_ptr<sjd::RatesEnvironment> re)
            { return option.getOptionValue(re); }, py::arg("rates_environment"),
            "The option alone, without the premium cashflow.");

    py::enum_<sjd::AssetClass>(m, "AssetClass")
        .value("INTEREST_RATE", sjd::AssetClass::InterestRate)
        .value("FX", sjd::AssetClass::FX)
        .value("GOLD", sjd::AssetClass::Gold)
        .value("EQUITY", sjd::AssetClass::Equity)
        .value("PRECIOUS_METAL", sjd::AssetClass::PreciousMetal)
        .value("COMMODITY", sjd::AssetClass::Commodity)
        .value("ELECTRICITY", sjd::AssetClass::Electricity)
        .value("OIL_GAS", sjd::AssetClass::OilGas)
        .value("METALS", sjd::AssetClass::Metals)
        .value("AGRICULTURAL", sjd::AssetClass::Agricultural)
        .value("OTHER_COMMODITY", sjd::AssetClass::OtherCommodity);

    py::class_<sjd::SupervisoryFactors>(m, "SupervisoryFactors")
        .def("get_factor", &sjd::SupervisoryFactors::getFactor, py::arg("time"),
             "CEM add-on for a time to settlement in years.")
        .def_readonly("asset_class", &sjd::SupervisoryFactors::assetClass)
        .def_readonly("supervisory_factor", &sjd::SupervisoryFactors::supervisoryFactor);

    m.def(
        "reg_cap_factors", [](const std::string& description)
        { return sjd::supervisoryFactors(description); }, py::arg("description"));

    py::class_<sjd::CounterpartyCredit>(m, "CounterpartyCredit")
        .def(py::init<double, double>(), py::arg("risk_weight"), py::arg("capital_ratio") = 0.08)
        .def("risk_weight", [](const sjd::CounterpartyCredit& credit) { return credit.riskWeight; })
        .def("capital_ratio", [](const sjd::CounterpartyCredit& credit) { return credit.capitalRatio; })
        .def("counterparty_weighting",
             [](const sjd::CounterpartyCredit& credit) { return credit.riskWeight; })
        .def("capital_charge", [](const sjd::CounterpartyCredit& credit) { return credit.capitalRatio; });
    m.attr("RegCapCreditParameters") = m.attr("CounterpartyCredit");

    auto creditCurve = py::class_<sjd::CVA, std::shared_ptr<sjd::CVA>>(
        m, "CreditCurve",
        "Default probability and loss given default. sjd.stats uses these curves for CVA and DVA.");
    bindValidation(creditCurve);
    creditCurve
        .def(
            "get_pd", [](sjd::CVA& curve, const py::object& day) { return curve.getPD(to_ql_date(day)); },
            py::arg("date"), "Cumulative probability of default from the anchor to this date.")
        .def(
            "get_pd", [](sjd::CVA& curve, const py::object& start, const py::object& end)
            { return curve.getPD(to_ql_date(start), to_ql_date(end)); }, py::arg("start"), py::arg("end"),
            "Probability of default between the two dates. Zero when the interval is empty.")
        .def(
            "get_lgd", [](sjd::CVA& curve, const py::object& day) { return curve.getLGD(to_ql_date(day)); },
            py::arg("date"))
        .def(
            "extends_to", [](sjd::CVA& curve, const py::object& day)
            { return curve.extendsTo(to_ql_date(day)); }, py::arg("date"));

    py::class_<sjd::FlatPD, sjd::CVA, std::shared_ptr<sjd::FlatPD>>(
        m, "FlatDefaultProbability",
        "Constant annual conditional default probability. Cumulative PD is 1 - exp(-h t), "
        "with t measured in years of 365 days.")
        .def(py::init(
                 [](const py::object& anchor, double probability, double loss_given_default) {
                     return std::make_shared<sjd::FlatPD>(to_ql_date(anchor), probability,
                                                          loss_given_default);
                 }),
             py::arg("anchor"), py::arg("annual_probability"), py::arg("loss_given_default"))
        .def("set_pd", &sjd::FlatPD::setPD, py::arg("annual_probability"))
        .def("set_lgd", &sjd::FlatPD::setLGD, py::arg("loss_given_default"));

    py::class_<sjd::CumulativeDefaultProbability, sjd::CVA,
               std::shared_ptr<sjd::CumulativeDefaultProbability>>(
        m, "CumulativeDefaultProbability",
        "Cumulative default probability and loss given default, interpolated on year fractions.")
        .def(py::init(
                 [](const py::object& anchor, const py::iterable& dates,
                    const std::vector<double>& cumulative_pd, const std::vector<double>& loss_given_default)
                 {
                     return std::make_shared<sjd::CumulativeDefaultProbability>(
                         to_ql_date(anchor), to_ql_dates(dates), cumulative_pd, loss_given_default);
                 }),
             py::arg("anchor"), py::arg("dates"), py::arg("cumulative_pd"), py::arg("loss_given_default"));

    auto margin = py::class_<sjd::Margin, std::shared_ptr<sjd::Margin>>(m, "Margin");
    bindValidation(margin);
    margin.def("get_margin_balance", &sjd::Margin::getMarginBalance)
        .def("set_margin_balance", &sjd::Margin::setMarginBalance, py::arg("balance"));

    py::class_<sjd::SimpleMargin, sjd::Margin, std::shared_ptr<sjd::SimpleMargin>>(m, "SimpleMargin")
        .def(py::init(
                 [](const py::object& anchor, bool use_my_threshold, double my_threshold,
                    bool use_cpty_threshold, double cpty_threshold, double minimum_transfer_amount,
                    double rounding)
                 {
                     return std::make_shared<sjd::SimpleMargin>(
                         to_ql_date(anchor), use_my_threshold, my_threshold, use_cpty_threshold,
                         cpty_threshold, minimum_transfer_amount, rounding);
                 }),
             py::arg("anchor_date"), py::arg("use_my_threshold"), py::arg("my_threshold"),
             py::arg("use_cpty_threshold"), py::arg("cpty_threshold"), py::arg("minimum_transfer_amount"),
             py::arg("rounding"), "Variation-margin CSA. Initial margin is not part of this model.")
        .def("get_margin_call", &sjd::SimpleMargin::getMarginCall, py::arg("mtm"));

    py::class_<sjd::Portfolio, sjd::Contract, std::shared_ptr<sjd::Portfolio>>(m, "Portfolio")
        .def(py::init([](const py::list& contracts)
                      { return std::make_shared<sjd::Portfolio>(contracts_from(contracts)); }),
             py::arg("contracts"))
        .def(py::init([](const py::list& contracts, std::shared_ptr<sjd::Margin> margin)
                      { return std::make_shared<sjd::Portfolio>(contracts_from(contracts), margin); }),
             py::arg("contracts"), py::arg("margin"))
        .def(
            "contracts",
            [](sjd::Portfolio& portfolio) { return portfolio.getUnderlyingSingleInstrumentContracts(); },
            "The single instruments in the portfolio.")
        .def("is_margined", &sjd::Portfolio::isMargined)
        .def("get_margin_balance", &sjd::Portfolio::getMarginBalance)
        .def("set_margin_balance", &sjd::Portfolio::setMarginBalance, py::arg("balance"))
        .def(
            "get_margin_call", [](sjd::Portfolio& portfolio, std::shared_ptr<sjd::RatesEnvironment> re)
            { return portfolio.getMarginCall(re); }, py::arg("rates_environment"));

    py::class_<sjd::CapitalResult>(m, "CapitalResult")
        .def_readonly("ead", &sjd::CapitalResult::ead)
        .def_readonly("rwa", &sjd::CapitalResult::rwa)
        .def_readonly("capital", &sjd::CapitalResult::capital)
        .def_readonly("replacement", &sjd::CapitalResult::replacement)
        .def_readonly("net_gross_ratio", &sjd::CapitalResult::netGrossRatio)
        .def_property_readonly("components",
                               [](const sjd::CapitalResult& result)
                               {
                                   py::list components;
                                   for (const sjd::CapitalComponent& component : result.components)
                                   {
                                       py::dict entry;
                                       entry["settlement"] = from_ql_date(component.settlement);
                                       entry["volume"] = component.volume;
                                       entry["capped"] = component.capped;
                                       entry["mark_to_market"] = component.markToMarket;
                                       entry["add_on"] = component.addOn;
                                       components.append(entry);
                                   }
                                   return components;
                               });

    auto exposure = py::class_<sjd::CurrentExposureMethod, std::shared_ptr<sjd::CurrentExposureMethod>>(
        m, "CurrentExposureMethod",
        "Basel II current-exposure capital for one portfolio. The portfolio is not a subclass.");
    bindEnvironmentValidation(exposure);
    exposure
        .def(py::init(
                 [](std::shared_ptr<sjd::Portfolio> portfolio, const sjd::SupervisoryFactors& factors,
                    sjd::CounterpartyCredit credit, bool optimise) {
                     return std::make_shared<sjd::CurrentExposureMethod>(*portfolio, factors.assetClass,
                                                                         credit, optimise);
                 }),
             py::arg("portfolio"), py::arg("factors"), py::arg("credit"), py::arg("optimise") = false)
        .def(
            "capital",
            [](const sjd::CurrentExposureMethod& model, const sjd::Portfolio& portfolio,
               const py::object& margin, std::shared_ptr<sjd::RatesEnvironment> environment)
            {
                std::shared_ptr<sjd::Margin> held = margin.is_none()
                                                        ? std::shared_ptr<sjd::Margin>()
                                                        : margin.cast<std::shared_ptr<sjd::Margin>>();
                return model.capital(portfolio, held.get(), environment);
            },
            py::arg("portfolio"), py::arg("margin"), py::arg("rates_environment"))
        .def("netting_set_contracts", &sjd::CurrentExposureMethod::nettingSetContracts)
        .def(
            "netting_sets",
            [](const sjd::CurrentExposureMethod& model)
            {
                sjd::Result<std::vector<sjd::NettingSet>> describedSets = model.nettingSets();
                if (!describedSets.ok())
                    throw sjd::ValidationError(describedSets.report.toString(), describedSets.report);
                py::list described;
                for (const sjd::NettingSet& netting : describedSets.value)
                {
                    py::dict entry;
                    entry["settlement"] = from_ql_date(netting.settlement);
                    entry["volume"] = netting.volume;
                    entry["has_price_exposure"] = netting.hasPriceExposure;
                    entry["capped"] = netting.capped;
                    entry["cap"] = netting.cap;
                    described.append(entry);
                }
                return described;
            },
            "Static netting sets. Mark-to-market along a path is simulation.by_netting_set.");

    auto pathGenerator =
        py::class_<sjd::GbmPathGenerator, std::shared_ptr<sjd::GbmPathGenerator>>(m, "GbmPathGenerator");
    bindValidation(pathGenerator);
    pathGenerator
        .def(py::init([](std::shared_ptr<sjd::RatesEnvironment> environment)
                      { return std::make_shared<sjd::GbmPathGenerator>(environment); }),
             py::arg("rates_environment"))
        .def("factors", &sjd::GbmPathGenerator::factors, "The number of random factors. This model has one.")
        .def("set_random_seed", &sjd::GbmPathGenerator::setRandomSeed, py::arg("seed"),
             "Reseed the generator. The next simulated paths repeat for the same seed.");
    m.attr("SingleFactorEconomicScenarioGenerator") = pathGenerator;

    auto evolverClass =
        py::class_<sjd::PortfolioEvolver, std::shared_ptr<sjd::PortfolioEvolver>>(m, "PortfolioEvolver");
    bindValidation(evolverClass);
    evolverClass
        .def(py::init(&make_evolver), py::arg("portfolio"), py::arg("path_generator"),
             py::arg("simulation_dates"), py::kw_only(), py::arg("holding_period") = 0,
             py::arg("holding_period_calendar") = py::none(),
             "With a holding period, a margined portfolio also simulates the date "
             "holding_period business days before each simulation date.")
        .def_property_readonly(
            "simulation_dates",
            [](sjd::PortfolioEvolver& evolver) { return py_dates(evolver.getSimulationDates()); },
            "The requested dates plus any margin reset dates.")
        .def_property_readonly("exposure_dates", [](sjd::PortfolioEvolver& evolver)
                               { return py_dates(evolver.getExposureDates()); })
        .def(
            "value_spot_path",
            [](sjd::PortfolioEvolver& evolver, const std::vector<double>& spots, bool by_trade,
               bool by_netting_set, const py::object& by_structure)
            {
                FlushLogs flush;
                const bool bySet = nettingSetsRequested(by_netting_set, by_structure);
                const auto setupStarted = std::chrono::steady_clock::now();
                if (!evolver.prepare())
                    throwIfNoPaths(evolver.validationReport());
                const long long setupMs = elapsedMilliseconds(setupStarted);
                const auto pathsStarted = std::chrono::steady_clock::now();
                sjd::SimulatedPaths simulated = evolver.valueSpotPath(spots, by_trade, bySet);
                const long long pathsMs = elapsedMilliseconds(pathsStarted);
                if (simulated.paths == 0)
                    throwIfNoPaths(evolver.validationReport());
                require_structures(bySet, simulated);
                Simulation result = simulation_from(simulated, false);
                py::dict inputs;
                inputs["spots"] = spots.size();
                inputs["by_trade"] = by_trade;
                inputs["by_netting_set"] = bySet;
                result.metadata = runMetadata(evolver, nullptr, nullptr, nullptr, inputs, setupMs, pathsMs);
                return result;
            },
            py::arg("spots"), py::arg("by_trade") = false, py::arg("by_netting_set") = false,
            py::arg("by_structure") = py::none(),
            "Mark the book to this forward path. One spot per simulation date. "
            "by_netting_set values each CEM netting set. by_structure is the same flag.")
        .def(
            "value_spot_paths",
            [](sjd::PortfolioEvolver& evolver,
               const py::array_t<double, py::array::c_style | py::array::forcecast>& spots, bool by_trade,
               bool by_netting_set, const py::object& by_structure, size_t threads)
            {
                const bool bySet = nettingSetsRequested(by_netting_set, by_structure);
                FlushLogs flush;
                if (spots.ndim() != 2)
                    throw py::value_error("spots must be shaped [paths, steps]");
                const py::ssize_t paths = spots.shape(0);
                const py::ssize_t steps = spots.shape(1);
                std::vector<double> flat(static_cast<size_t>(paths * steps));
                const double* data = spots.data();
                std::copy(data, data + flat.size(), flat.begin());
                sjd::SimulatedPaths simulated;
                long long setupMs = 0;
                long long pathsMs = 0;
                {
                    py::gil_scoped_release release;
                    const auto setupStarted = std::chrono::steady_clock::now();
                    const bool ready = evolver.prepare();
                    setupMs = elapsedMilliseconds(setupStarted);
                    if (!ready)
                    {
                        simulated.paths = 0;
                    }
                    else
                    {
                        const auto pathsStarted = std::chrono::steady_clock::now();
                        simulated = evolver.valueSpotPaths(flat, static_cast<size_t>(paths), by_trade, bySet,
                                                           threads);
                        pathsMs = elapsedMilliseconds(pathsStarted);
                    }
                }
                if (simulated.paths == 0)
                    throwIfNoPaths(evolver.validationReport());
                if (static_cast<py::ssize_t>(simulated.steps) != steps)
                    throw py::value_error("spots steps must match the evolver simulation dates");
                require_structures(bySet, simulated);
                Simulation result = simulation_from(simulated, false);
                py::dict inputs;
                inputs["paths"] = paths;
                inputs["steps"] = steps;
                inputs["threads"] = threads;
                inputs["by_trade"] = by_trade;
                inputs["by_netting_set"] = bySet;
                result.metadata = runMetadata(evolver, nullptr, nullptr, nullptr, inputs, setupMs, pathsMs);
                return result;
            },
            py::arg("spots"), py::arg("by_trade") = false, py::arg("by_netting_set") = false,
            py::arg("by_structure") = py::none(), py::arg("threads") = 0,
            "Mark the book to many forward paths. spots is shaped [paths, steps]. "
            "Reuse the same spots across candidate portfolios for common random numbers. "
            "threads 0 uses one thread per core, and never more threads than paths.");

    py::class_<Simulation>(m, "Simulation",
                           "Simulated paths. Arrays are numpy, indexed [path, step], "
                           "by_trade [path, step, trade] and by_netting_set [path, step, set].")
        .def_readonly("dates", &Simulation::dates, "Every simulated date, including margin reset dates.")
        .def_readonly("exposure_steps", &Simulation::exposure_steps,
                      "Step of each requested simulation date.")
        .def_readonly("margin_reset_steps", &Simulation::margin_reset_steps,
                      "Step whose margin balance applies at each exposure step. Equal to "
                      "exposure_steps without a holding period.")
        .def_readonly("spot", &Simulation::spot, "Forward for each simulation date, read on that date.")
        .def_readonly("mtm", &Simulation::mtm, "Portfolio value.")
        .def_readonly("margin", &Simulation::margin,
                      "Margin balance, or None when the portfolio is not margined. "
                      "With aggregates, the mean margin.")
        .def_readonly("ee", &Simulation::ee,
                      "Expected exposure of each step, or None unless aggregates was set.")
        .def_readonly(
            "pfe", &Simulation::pfe,
            "95% potential future exposure of the positive part, or None unless aggregates was set.")
        .def_readonly("mean_capital", &Simulation::mean_capital,
                      "Mean CEM capital on the exposure dates, or None when the run has no netting sets.")
        .def_readonly("aggregated", &Simulation::aggregated,
                      "True when spot, mtm and margin are per-step profiles rather than path cubes.")
        .def_readonly("by_trade", &Simulation::by_trade,
                      "Value of each underlying contract, or None unless by_trade was requested.")
        .def_property_readonly(
            "by_netting_set", [](const Simulation& simulation) { return simulation.by_structure; },
            "Value of each CEM netting set, or None unless by_netting_set was requested.")
        .def_property_readonly(
            "by_structure", [](const Simulation& simulation) { return simulation.by_structure; },
            "Deprecated alias of by_netting_set.")
        .def_readonly("paths", &Simulation::paths)
        .def_readonly("steps", &Simulation::steps)
        .def_readonly("trades", &Simulation::trades)
        .def_readonly("structures", &Simulation::structures)
        .def_readonly("antithetic", &Simulation::antithetic,
                      "Path i and path i + paths // 2 use opposite normals.")
        .def_readonly("seed", &Simulation::seed,
                      "Seed used to draw the paths. None when the spots were supplied. "
                      "Pass it back to simulate to repeat the arrays.")
        .def_readonly("metadata", &Simulation::metadata,
                      "Version, git commit, build type, setup and path timings, validation "
                      "warnings, and the inputs of this run.")
        .def("__repr__",
             [](const Simulation& simulation)
             {
                 return "<Simulation paths=" + std::to_string(simulation.paths) +
                        " steps=" + std::to_string(simulation.steps) +
                        " trades=" + std::to_string(simulation.trades) +
                        " structures=" + std::to_string(simulation.structures) + ">";
             });

    m.def(
        "simulate",
        [](const py::object& book, std::shared_ptr<sjd::RatesEnvironment> environment,
           const py::iterable& simulation_dates, unsigned long paths, const py::object& seed, bool antithetic,
           bool by_trade, bool by_netting_set, const py::object& by_structure, size_t holding_period,
           const py::object& holding_period_calendar, size_t threads, const std::string& sampling,
           bool aggregates, bool float32)
        {
            FlushLogs flush;
            const bool bySet = nettingSetsRequested(by_netting_set, by_structure);
            sjd::PathSampling sampler = sjd::PathSampling::Pseudo;
            if (sampling == "sobol")
                sampler = sjd::PathSampling::Sobol;
            else if (sampling != "pseudo")
                throw py::value_error("sampling must be 'pseudo' or 'sobol'");
            const ResolvedBook resolved = resolveBook(book);
            std::shared_ptr<sjd::SingleFactorEconomicScenarioGenerator> generator(
                std::make_shared<sjd::SingleFactorEconomicScenarioGenerator>(environment));
            if (seed.is_none())
                generator->setRandomSeed(static_cast<long>(std::random_device{}()));
            else
                generator->setRandomSeed(seed.cast<long>());
            const long usedSeed = generator->getRandomSeed();
            std::shared_ptr<sjd::PortfolioEvolver> evolver =
                make_evolver(book, generator, simulation_dates, holding_period, holding_period_calendar);
            SJD_LOG_INFO("simulation start paths=" + std::to_string(paths) +
                         " dates=" + std::to_string(evolver->getSimulationDates().size()) + " antithetic=" +
                         std::string(antithetic ? "true" : "false") + " seed=" + std::to_string(usedSeed));
            sjd::SimulationOptions options;
            options.antithetic = antithetic;
            options.byTrade = by_trade;
            options.byNettingSet = bySet;
            options.threads = threads;
            options.sampling = sampler;
            options.aggregates = aggregates;
            options.capital = aggregates && !resolved.nettingSets.empty();
            if (options.capital && py::hasattr(book, "capital_model"))
            {
                std::shared_ptr<sjd::CurrentExposureMethod> model =
                    book.attr("capital_model").cast<std::shared_ptr<sjd::CurrentExposureMethod>>();
                const sjd::CounterpartyCredit& credit = model->counterparty();
                options.capitalScale = credit.capitalRatio * credit.riskWeight;
            }
            sjd::SimulatedPaths simulated;
            long long setupMs = 0;
            long long pathsMs = 0;
            {
                py::gil_scoped_release release;
                const auto setupStarted = std::chrono::steady_clock::now();
                const bool ready = evolver->prepare();
                setupMs = elapsedMilliseconds(setupStarted);
                if (!ready)
                {
                    simulated.paths = 0;
                }
                else
                {
                    const auto pathsStarted = std::chrono::steady_clock::now();
                    simulated = evolver->simulatePaths(paths, options);
                    pathsMs = elapsedMilliseconds(pathsStarted);
                }
            }
            if (simulated.paths == 0)
                throwIfNoPaths(evolver->validationReport());
            SJD_LOG_INFO("simulation end paths=" + std::to_string(simulated.paths) + " steps=" +
                         std::to_string(simulated.steps) + " trades=" + std::to_string(simulated.trades) +
                         " setup_ms=" + std::to_string(setupMs) + " paths_ms=" + std::to_string(pathsMs));
            require_structures(bySet, simulated);
            Simulation result = simulation_from(simulated, float32);
            result.seed = py::int_(usedSeed);
            py::dict inputs;
            inputs["paths"] = paths;
            inputs["seed"] = usedSeed;
            inputs["antithetic"] = antithetic;
            inputs["by_trade"] = by_trade;
            inputs["by_netting_set"] = bySet;
            inputs["holding_period"] = holding_period;
            inputs["threads"] = threads;
            inputs["sampling"] = sampling;
            inputs["aggregates"] = aggregates;
            inputs["float32"] = float32;
            result.metadata =
                runMetadata(*evolver, resolved.portfolio, generator, environment, inputs, setupMs, pathsMs);
            return result;
        },
        py::arg("portfolio"), py::arg("rates_environment"), py::arg("simulation_dates"),
        py::arg("paths") = 10000, py::kw_only(), py::arg("seed") = py::none(), py::arg("antithetic") = true,
        py::arg("by_trade") = false, py::arg("by_netting_set") = false, py::arg("by_structure") = py::none(),
        py::arg("holding_period") = 0, py::arg("holding_period_calendar") = py::none(),
        py::arg("threads") = 0, py::arg("sampling") = "pseudo", py::arg("aggregates") = false,
        py::arg("float32") = false,
        "Simulate the portfolio along single-factor paths and return every value.\n\n"
        "The first simulation date must be the anchor date, and the forward must have a "
        "stochastic roll. With antithetic paths an odd count is rounded up. Omit seed and "
        "one is drawn from the system; simulation.seed is the value that repeats the "
        "arrays. simulation.metadata records the build, timings, warnings and inputs. "
        "threads 0 uses one thread per core. One thread and many threads write the same "
        "numbers. sampling is 'pseudo' or 'sobol'. aggregates keeps per-step profiles "
        "instead of the path cubes. float32 casts the arrays on the way out. "
        "A holding period on a margined portfolio adds a margin reset date "
        "holding_period business days before each simulation date. Raises "
        "ValidationError if the portfolio or market cannot be simulated.");

    m.def(
        "set_log_level", [](const std::string& level) { sjd::log::logger().setLevel(levelFromName(level)); },
        py::arg("level"),
        "Set the C++ logger. Python code should call sjd.set_log_level, which also sets the sjd logger.");

    m.attr("__version__") = SJD_VERSION;
    m.def("build_info", &libraryBuildInfo,
          "Compiler, build type, QuantLib and Boost versions, and the git commit at configure time.");
}
