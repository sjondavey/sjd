#pragma once

#include <sjd/tools/Validation.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace sjd
{

    // Failures that are not validation issues. See docs/error-handling.md.
    class Error : public std::runtime_error
    {
    public:
        Error(std::string code, std::string message)
            : std::runtime_error(std::move(message)), code_(std::move(code))
        {
        }

        const std::string& code() const { return code_; }

    private:
        std::string code_;
    };

    class ConfigurationError : public Error
    {
    public:
        explicit ConfigurationError(std::string message) : Error("CONFIGURATION", std::move(message)) {}
    };

    class NumericalError : public Error
    {
    public:
        explicit NumericalError(std::string message) : Error("NUMERICAL", std::move(message)) {}
    };

    // A convenience function validated, found the object unusable, and refused to calculate.
    class ValidationError : public Error
    {
    public:
        ValidationError(std::string message, ValidationReport report);
        const ValidationReport& report() const;

    private:
        ValidationReport report_;
    };

}

// Always checked. Throws sjd::Error with code REQUIRE.
#define SJD_REQUIRE(condition)                                                                               \
    do                                                                                                       \
    {                                                                                                        \
        if (!(condition))                                                                                    \
        {                                                                                                    \
            throw ::sjd::Error("REQUIRE", "Requirement failed: " #condition);                                \
        }                                                                                                    \
    } while (false)

// Checked in Debug, where a failure throws sjd::Error with code ASSERT.
// Compiled out in Release so a hot path does not test it.
#ifndef NDEBUG
#define SJD_ASSERT(condition)                                                                                \
    do                                                                                                       \
    {                                                                                                        \
        if (!(condition))                                                                                    \
        {                                                                                                    \
            throw ::sjd::Error("ASSERT", "Assertion failed: " #condition);                                   \
        }                                                                                                    \
    } while (false)
#else
#define SJD_ASSERT(condition)                                                                                \
    do                                                                                                       \
    {                                                                                                        \
    } while (false)
#endif
