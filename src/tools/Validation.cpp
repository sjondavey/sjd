#include <sjd/tools/Validation.h>

#include <sstream>
#include <utility>

using namespace std;

namespace sjd
{
    namespace
    {

        std::string joinPath(const std::vector<std::string>& path)
        {
            std::string joined;
            for (size_t i = 0; i < path.size(); ++i)
            {
                if (i > 0)
                {
                    joined += '/';
                }
                joined += path[i];
            }
            return joined;
        }

        std::string jsonEscape(const std::string& text)
        {
            std::string escaped;
            escaped.reserve(text.size());
            for (char character : text)
            {
                switch (character)
                {
                case '\\':
                    escaped += "\\\\";
                    break;
                case '"':
                    escaped += "\\\"";
                    break;
                case '\n':
                    escaped += "\\n";
                    break;
                case '\r':
                    escaped += "\\r";
                    break;
                case '\t':
                    escaped += "\\t";
                    break;
                default:
                    escaped += character;
                    break;
                }
            }
            return escaped;
        }

        const char* severityName(ValidationSeverity severity)
        {
            switch (severity)
            {
            case ValidationSeverity::INFO:
                return "INFO";
            case ValidationSeverity::WARNING:
                return "WARNING";
            case ValidationSeverity::ERROR:
                return "ERROR";
            }
            return "ERROR";
        }

        ValidationReport filterSeverity(const std::vector<ValidationIssue>& issues,
                                        ValidationSeverity severity)
        {
            std::vector<ValidationIssue> selected;
            for (const ValidationIssue& issue : issues)
            {
                if (issue.severity == severity)
                {
                    selected.push_back(issue);
                }
            }
            return ValidationReport(selected);
        }

    }

    ValidationIssue::ValidationIssue(std::string codeInput,
                                     std::string messageInput,
                                     ValidationSeverity severityInput,
                                     std::vector<std::string> pathInput)
        : code(std::move(codeInput)), message(std::move(messageInput)), severity(severityInput),
          path(std::move(pathInput))
    {
    }

    ValidationReport::ValidationReport() {}

    ValidationReport::ValidationReport(std::vector<ValidationIssue> issuesInput)
        : issues(std::move(issuesInput))
    {
    }

    bool ValidationReport::isOK() const
    {
        for (const ValidationIssue& issue : issues)
        {
            if (issue.severity == ValidationSeverity::ERROR)
            {
                return false;
            }
        }
        return true;
    }

    const std::vector<ValidationIssue>& ValidationReport::getIssues() const
    {
        return issues;
    }

    ValidationReport ValidationReport::merge(const ValidationReport& other) const
    {
        std::vector<ValidationIssue> combined = issues;
        combined.insert(combined.end(), other.issues.begin(), other.issues.end());
        return ValidationReport(std::move(combined));
    }

    ValidationReport ValidationReport::errors() const
    {
        return filterSeverity(issues, ValidationSeverity::ERROR);
    }

    ValidationReport ValidationReport::warnings() const
    {
        return filterSeverity(issues, ValidationSeverity::WARNING);
    }

    std::string ValidationReport::toString() const
    {
        std::ostringstream stream;
        for (size_t i = 0; i < issues.size(); ++i)
        {
            if (i > 0)
            {
                stream << '\n';
            }
            stream << joinPath(issues[i].path) << ": " << issues[i].code << ": " << issues[i].message;
        }
        return stream.str();
    }

    std::string ValidationReport::toJson() const
    {
        std::ostringstream stream;
        stream << '[';
        for (size_t i = 0; i < issues.size(); ++i)
        {
            if (i > 0)
            {
                stream << ',';
            }
            const ValidationIssue& issue = issues[i];
            stream << "{\"code\":\"" << jsonEscape(issue.code) << "\",\"message\":\""
                   << jsonEscape(issue.message) << "\",\"severity\":\"" << severityName(issue.severity)
                   << "\",\"path\":[";
            for (size_t element = 0; element < issue.path.size(); ++element)
            {
                if (element > 0)
                {
                    stream << ',';
                }
                stream << '"' << jsonEscape(issue.path[element]) << '"';
            }
            stream << "]}";
        }
        stream << ']';
        return stream.str();
    }

    ValidationReport ValidationReport::withIssue(const ValidationIssue& issue) const
    {
        std::vector<ValidationIssue> updatedIssues = issues;
        updatedIssues.push_back(issue);
        return ValidationReport(updatedIssues);
    }

    ValidationReport ValidationReport::withPrefixedPath(const std::string& pathElement) const
    {
        std::vector<ValidationIssue> updatedIssues = issues;
        for (ValidationIssue& issue : updatedIssues)
        {
            issue.path.insert(issue.path.begin(), pathElement);
        }
        return ValidationReport(updatedIssues);
    }

    void ValidationLog::clear()
    {
        issues.clear();
    }

    void ValidationLog::add(ValidationIssue issue)
    {
        issues.push_back(std::move(issue));
    }

    const std::vector<ValidationIssue>& ValidationLog::getIssues() const
    {
        return issues;
    }

    ValidationReport ValidationLog::snapshot() const
    {
        return ValidationReport(issues);
    }

    IssueTracker::IssueTracker(std::string classNameInput) : className(std::move(classNameInput)) {}

    const std::string& IssueTracker::getClassName() const
    {
        return className;
    }

    void IssueTracker::clear()
    {
        log.clear();
    }

    void IssueTracker::record(std::string_view code,
                              std::string_view message,
                              const std::vector<std::string>& path,
                              ValidationSeverity severity)
    {
        log.add(ValidationIssue(std::string(code), std::string(message), severity, path));
    }

    void IssueTracker::recordWarning(std::string_view code,
                                     std::string_view message,
                                     const std::vector<std::string>& path)
    {
        record(code, message, path, ValidationSeverity::WARNING);
    }

    void IssueTracker::recordInfo(std::string_view code,
                                  std::string_view message,
                                  const std::vector<std::string>& path)
    {
        record(code, message, path, ValidationSeverity::INFO);
    }

    void IssueTracker::absorb(const ValidationReport& child, const std::vector<std::string>& prefix)
    {
        for (ValidationIssue issue : child.getIssues())
        {
            issue.path.insert(issue.path.begin(), prefix.begin(), prefix.end());
            log.add(std::move(issue));
        }
    }

    bool IssueTracker::getHasErrors() const
    {
        for (const ValidationIssue& issue : log.getIssues())
        {
            if (issue.severity == ValidationSeverity::ERROR)
            {
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> IssueTracker::getErrorMessages() const
    {
        std::vector<std::string> messages;
        for (const ValidationIssue& issue : log.getIssues())
        {
            if (issue.severity == ValidationSeverity::ERROR)
            {
                messages.push_back(className + ": " + issue.message);
            }
        }
        return messages;
    }

    std::string IssueTracker::getErrorMessagesAsString() const
    {
        const std::vector<std::string> messages = getErrorMessages();
        std::string allErrorMessages;
        for (size_t i = 0; i < messages.size(); ++i)
        {
            allErrorMessages += messages[i];
            if (i + 1 < messages.size())
            {
                allErrorMessages += ", ";
            }
        }
        return allErrorMessages;
    }

    ValidationReport IssueTracker::snapshot() const
    {
        return log.snapshot();
    }

    Validatable::Validatable() : errorTracking(std::make_shared<IssueTracker>("")) {}

    ValidationReport Validatable::validate()
    {
        resetErrorMessages();
        runChecks();
        return validationReport();
    }

    ValidationReport Validatable::validateReport()
    {
        return validate();
    }

    void Validatable::runChecks() {}

    bool Validatable::isValid() const
    {
        return !hasErrors();
    }

    bool Validatable::isOK() const
    {
        return isValid();
    }

    bool Validatable::hasErrors() const
    {
        return errorTracking->getHasErrors();
    }

    std::vector<std::string> Validatable::getErrorMessages() const
    {
        return errorTracking->getErrorMessages();
    }

    std::string Validatable::getErrorMessagesAsString() const
    {
        return errorTracking->getErrorMessagesAsString();
    }

    ValidationReport Validatable::validationReport() const
    {
        return errorTracking->snapshot();
    }

    void Validatable::resetErrorMessages()
    {
        errorTracking->clear();
    }

    void Validatable::recordIssue(std::string_view code,
                                  std::string_view message,
                                  const std::vector<std::string>& tail)
    {
        std::vector<std::string> path(1, validationRoot());
        path.insert(path.end(), tail.begin(), tail.end());
        errorTracking->record(code, message, path, ValidationSeverity::ERROR);
    }

    void Validatable::recordWarning(std::string_view code,
                                    std::string_view message,
                                    const std::vector<std::string>& tail)
    {
        std::vector<std::string> path(1, validationRoot());
        path.insert(path.end(), tail.begin(), tail.end());
        errorTracking->recordWarning(code, message, path);
    }

    void Validatable::recordInfo(std::string_view code,
                                 std::string_view message,
                                 const std::vector<std::string>& tail)
    {
        std::vector<std::string> path(1, validationRoot());
        path.insert(path.end(), tail.begin(), tail.end());
        errorTracking->recordInfo(code, message, path);
    }

    void Validatable::absorbChildReport(const ValidationReport& child, const std::vector<std::string>& extra)
    {
        std::vector<std::string> prefix(1, validationRoot());
        prefix.insert(prefix.end(), extra.begin(), extra.end());
        errorTracking->absorb(child, prefix);
    }
}
