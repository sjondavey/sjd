#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace sjd
{

    // Base for failures that cannot be recorded as validation issues.
    // Step 7 extends this into the full error policy.
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

}
