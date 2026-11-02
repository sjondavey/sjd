#include <sjd/tools/Validation.h>

#include <utility>

using namespace std;

namespace sjd
{
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

    void IssueTracker::record(const std::string& code,
                              const std::string& message,
                              const std::vector<std::string>& path)
    {
        log.add(ValidationIssue(code, message, ValidationSeverity::ERROR, path));
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

    void Validatable::recordIssue(const std::string& code,
                                  const std::string& message,
                                  const std::vector<std::string>& tail)
    {
        std::vector<std::string> path(1, validationRoot());
        path.insert(path.end(), tail.begin(), tail.end());
        errorTracking->record(code, message, path);
    }

    void Validatable::absorbChildReport(const ValidationReport& child, const std::vector<std::string>& extra)
    {
        std::vector<std::string> prefix(1, validationRoot());
        prefix.insert(prefix.end(), extra.begin(), extra.end());
        errorTracking->absorb(child, prefix);
    }
}
