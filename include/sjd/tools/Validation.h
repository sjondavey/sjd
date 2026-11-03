#pragma once

#include <sjd/tools/IssueCodes.h>

#include <memory>
#include <string>
#include <string_view>
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

        // False when any issue has severity ERROR. Warnings and notes do not fail it.
        bool isOK() const;
        const std::vector<ValidationIssue>& getIssues() const;

        ValidationReport merge(const ValidationReport& other) const;
        ValidationReport errors() const;
        ValidationReport warnings() const;

        // One line per issue: path: code: message. Path elements are joined with '/'.
        std::string toString() const;
        std::string toJson() const;

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
    class IssueTracker
    {
    public:
        explicit IssueTracker(std::string classNameInput);

        const std::string& getClassName() const;
        void clear();
        void record(std::string_view code,
                    std::string_view message,
                    const std::vector<std::string>& path,
                    ValidationSeverity severity = ValidationSeverity::ERROR);
        void recordWarning(std::string_view code,
                           std::string_view message,
                           const std::vector<std::string>& path);
        void recordInfo(std::string_view code,
                        std::string_view message,
                        const std::vector<std::string>& path);
        void absorb(const ValidationReport& child, const std::vector<std::string>& prefix);

        bool getHasErrors() const;
        // Deprecated. The old "className: message" text, errors only, until Step 10.
        std::vector<std::string> getErrorMessages() const;
        std::string getErrorMessagesAsString() const;
        ValidationReport snapshot() const;

    private:
        std::string className;
        ValidationLog log;
    };

    // Checks live in runChecks(). validate() clears the tracker, runs them, and returns
    // the report. isValid() and isOK() only read that report; neither one re-runs checks.
    // isOK() is the deprecated name of isValid().
    //
    // A constructor names the tracker by assigning errorTracking, for example
    // errorTracking = std::make_shared<IssueTracker>("DiscountCurve"). That name prefixes
    // getErrorMessages(). New code should read validationReport().toString().
    class Validatable
    {
    public:
        virtual ~Validatable() = default;

        ValidationReport validate();
        // Deprecated wrapper. Calls validate().
        ValidationReport validateReport();

        bool isValid() const;
        // Deprecated alias of isValid(). Does not run checks.
        bool isOK() const;

        bool hasErrors() const;
        // Deprecated. Prefer validationReport().toString(). Kept so existing tests and
        // the Python bindings can keep searching the old "className: message" text.
        std::vector<std::string> getErrorMessages() const;
        std::string getErrorMessagesAsString() const;
        virtual ValidationReport validationReport() const;
        void resetErrorMessages();

    protected:
        Validatable();

        virtual void runChecks();
        virtual std::string validationRoot() const = 0;

        void recordIssue(std::string_view code,
                         std::string_view message,
                         const std::vector<std::string>& tail);
        void recordWarning(std::string_view code,
                           std::string_view message,
                           const std::vector<std::string>& tail);
        void recordInfo(std::string_view code,
                        std::string_view message,
                        const std::vector<std::string>& tail);
        void absorbChildReport(const ValidationReport& child, const std::vector<std::string>& extra);

        std::shared_ptr<IssueTracker> errorTracking;
    };
}
