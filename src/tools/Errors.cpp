#include <sjd/tools/Errors.h>

namespace sjd
{

    ValidationError::ValidationError(std::string message, ValidationReport report)
        : Error("VALIDATION", std::move(message)), report_(std::move(report))
    {
    }

    const ValidationReport& ValidationError::report() const
    {
        return report_;
    }

}
