#ifndef SJD_VALIDATION_INCLUDED
#define SJD_VALIDATION_INCLUDED
#pragma once

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

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

    // The issue store and its read-only views, for every class that validates itself.
    // Checks record issues with recordIssue; the public methods here only read them.
    // Each issue path starts with validationRoot().
    //
    // A constructor names the tracker by assigning errorTracking, e.g.
    // errorTracking.reset(new IssueTracker("DiscountCurve")). The class name prefixes
    // every message from getErrorMessages().
    //
    // isOK() is declared by each subclass and comes in two styles:
    //  - Contracts: validate() runs the checks and isOK() only reads the result.
    //  - Everything else (rate sources, generators, PortfolioEvolver, Margin, CVA,
    //    insurance, ContractStressor, Path): isOK() clears and re-runs every check.
    // hasErrors() never re-checks, so use it for a pure read in either style.
    class Validatable
    {
    public:
        virtual ~Validatable() = default;

        bool hasErrors() const;
        std::vector<std::string> getErrorMessages() const;
        std::string getErrorMessagesAsString() const;
        virtual ValidationReport validationReport() const;
        void resetErrorMessages();

    protected:
        Validatable();

        virtual std::string validationRoot() const = 0;
        void recordIssue(const std::string& code,
                         const std::string& message,
                         const std::vector<std::string>& tail);
        void absorbChildReport(const ValidationReport& child,
                               const std::vector<std::string>& extra);

        boost::shared_ptr<IssueTracker> errorTracking;
    };
}

#endif
