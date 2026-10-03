#ifndef SJD_VALIDATION_INCLUDED
#define SJD_VALIDATION_INCLUDED
#pragma once

#include <string>
#include <vector>

namespace sjd
{
    enum class ValidationSeverity
    {
        INFO,
        WARNING,
        ERROR
    };

    struct ValidationIssue
    {
        ValidationIssue(std::string codeInput,
                        std::string messageInput,
                        ValidationSeverity severityInput,
                        std::vector<std::string> pathInput);

        std::string code;
        std::string message;
        ValidationSeverity severity;
        std::vector<std::string> path;
    };

    class ValidationReport
    {
    public:
        ValidationReport();
        explicit ValidationReport(std::vector<ValidationIssue> issues);

        bool isOK() const;
        const std::vector<ValidationIssue>& getIssues() const;

        ValidationReport withIssue(const ValidationIssue& issue) const;
        ValidationReport withPrefixedPath(const std::string& pathElement) const;

    private:
        std::vector<ValidationIssue> issues;
    };

    class ValidationLog
    {
    public:
        void clear();
        void add(ValidationIssue issue);
        const std::vector<ValidationIssue>& getIssues() const;
        ValidationReport snapshot() const;

    private:
        std::vector<ValidationIssue> issues;
    };

    // One store per object. Issues are recorded at the failing check.
    // isOK() and getErrorMessages() on domain objects are views of this log.
    class IssueTracker
    {
    public:
        explicit IssueTracker(std::string classNameInput);

        const std::string& getClassName() const;
        void clear();
        void record(const std::string& code,
                    const std::string& message,
                    const std::vector<std::string>& path);
        void absorb(const ValidationReport& child, const std::vector<std::string>& prefix);

        bool getHasErrors() const;
        std::vector<std::string> getErrorMessages() const;
        std::string getErrorMessagesAsString() const;
        ValidationReport snapshot() const;

    private:
        std::string className;
        ValidationLog log;
    };
}

#endif
