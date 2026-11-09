#pragma once

#include <string>

namespace sjd::log
{

    enum class Level
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Off = 5
    };

    // The process-wide logger. It starts at Info with no sink, so a library call
    // is silent until a tool attaches stderr or the Python module installs its bridge.
    class Logger
    {
    public:
        void setLevel(Level level);
        Level getLevel() const;
        bool enabled(Level level) const;
        void write(Level level, std::string message);
        void attachStderrSink();
    };

    Logger& logger();

    // Optional consumer, used by the Python module. Called after the record is accepted.
    // Not invoked for records compiled out or below the current level.
    using RecordHandler = void (*)(Level level, const std::string& message);
    void setRecordHandler(RecordHandler handler);

}

// TRACE and DEBUG are removed from Release (NDEBUG), including their arguments,
// so a Monte Carlo loop does not build those strings. SPDLOG_ACTIVE_LEVEL in
// Log.cpp does the same for the spdlog calls themselves.
#if defined(NDEBUG)
#define SJD_LOG_TRACE(message) ((void)0)
#define SJD_LOG_DEBUG(message) ((void)0)
#else
#define SJD_LOG_TRACE(message)                                                                               \
    do                                                                                                       \
    {                                                                                                        \
        if (::sjd::log::logger().enabled(::sjd::log::Level::Trace))                                          \
            ::sjd::log::logger().write(::sjd::log::Level::Trace, (message));                                 \
    } while (false)
#define SJD_LOG_DEBUG(message)                                                                               \
    do                                                                                                       \
    {                                                                                                        \
        if (::sjd::log::logger().enabled(::sjd::log::Level::Debug))                                          \
            ::sjd::log::logger().write(::sjd::log::Level::Debug, (message));                                 \
    } while (false)
#endif

#define SJD_LOG_INFO(message)                                                                                \
    do                                                                                                       \
    {                                                                                                        \
        if (::sjd::log::logger().enabled(::sjd::log::Level::Info))                                           \
            ::sjd::log::logger().write(::sjd::log::Level::Info, (message));                                  \
    } while (false)

#define SJD_LOG_WARN(message)                                                                                \
    do                                                                                                       \
    {                                                                                                        \
        if (::sjd::log::logger().enabled(::sjd::log::Level::Warn))                                           \
            ::sjd::log::logger().write(::sjd::log::Level::Warn, (message));                                  \
    } while (false)

#define SJD_LOG_ERROR(message)                                                                               \
    do                                                                                                       \
    {                                                                                                        \
        if (::sjd::log::logger().enabled(::sjd::log::Level::Error))                                          \
            ::sjd::log::logger().write(::sjd::log::Level::Error, (message));                                 \
    } while (false)
