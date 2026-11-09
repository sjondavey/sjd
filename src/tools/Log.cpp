#include <sjd/tools/Log.h>

#ifndef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <mutex>

namespace sjd::log
{
    namespace
    {
        std::mutex& stateMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        std::shared_ptr<spdlog::logger>& spdlogLogger()
        {
            static auto logger = std::make_shared<spdlog::logger>("sjd");
            return logger;
        }

        std::atomic<int>& threshold()
        {
            static std::atomic<int> current{static_cast<int>(Level::Info)};
            return current;
        }

        RecordHandler& handler()
        {
            static RecordHandler current = nullptr;
            return current;
        }

        spdlog::level::level_enum toSpdlog(Level level)
        {
            switch (level)
            {
            case Level::Trace:
                return spdlog::level::trace;
            case Level::Debug:
                return spdlog::level::debug;
            case Level::Info:
                return spdlog::level::info;
            case Level::Warn:
                return spdlog::level::warn;
            case Level::Error:
                return spdlog::level::err;
            case Level::Off:
                return spdlog::level::off;
            }
            return spdlog::level::off;
        }

    }

    void Logger::setLevel(Level level)
    {
        std::lock_guard<std::mutex> lock(stateMutex());
        threshold().store(static_cast<int>(level), std::memory_order_relaxed);
        spdlogLogger()->set_level(toSpdlog(level));
    }

    Level Logger::getLevel() const
    {
        return static_cast<Level>(threshold().load(std::memory_order_relaxed));
    }

    bool Logger::enabled(Level level) const
    {
        return level != Level::Off && static_cast<int>(level) >= threshold().load(std::memory_order_relaxed);
    }

    void Logger::write(Level level, std::string message)
    {
        RecordHandler consumer = nullptr;
        {
            std::lock_guard<std::mutex> lock(stateMutex());
            if (level == Level::Off || static_cast<int>(level) < threshold().load(std::memory_order_relaxed))
                return;
            spdlogLogger()->log(toSpdlog(level), "{}", message);
            consumer = handler();
        }
        if (consumer)
            consumer(level, message);
    }

    void Logger::attachStderrSink()
    {
        std::lock_guard<std::mutex> lock(stateMutex());
        spdlogLogger()->sinks().push_back(std::make_shared<spdlog::sinks::stderr_sink_mt>());
    }

    Logger& logger()
    {
        static Logger instance;
        return instance;
    }

    void setRecordHandler(RecordHandler consumer)
    {
        std::lock_guard<std::mutex> lock(stateMutex());
        handler() = consumer;
    }

}
