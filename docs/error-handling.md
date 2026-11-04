# Error handling

One rule for every kind of failure. A class uses the mechanism that matches the
failure, and it does not invent a fourth one.

## 1. Validation issues

Bad inputs that can be found before calculating — a missing curve, a date
outside a pillar range, a negative strike, a structure that cannot be netted —
are validation issues. They are recorded on the object's `ValidationReport`
with a stable code, a message, a severity, and a path. This is the existing
`Validatable` design. The caller decides whether to stop.

## 2. Exceptions

A problem that makes an object impossible to construct throws. The types live
in `include/sjd/tools/Errors.h` and all derive from `sjd::Error`, which derives
from `std::runtime_error` and carries `code()`. QuantLib also has a type named
`Error`, so a file that imports both namespaces writes `sjd::Error`.

| Type | Code | Use |
| --- | --- | --- |
| `ConfigurationError` | `CONFIGURATION` | An unknown name or an option the type cannot accept, such as an unknown calendar or a calendar-month curve rolled with spot volatility. |
| `ValidationError` | `VALIDATION` | A convenience function validated its input, found it unusable, and will not calculate. It carries the `ValidationReport`. |
| `NumericalError` | `NUMERICAL` | A solver did not converge, or a strike and a delta could not be converted. |

`SJD_REQUIRE(condition)` is checked in every build. A failure throws `Error`
with code `REQUIRE`. Use it for a precondition that must hold in Release as
well as Debug.

## 3. Assertions

A programming error on a hot path — an index that the caller was required to
keep in range, or `getValue` used without a successful validation — is
`SJD_ASSERT`. In Debug a failure throws `Error` with code `ASSERT`. In Release
the check is compiled out, so a Monte Carlo loop does not pay for it.

A formula (`Black76Option`, `AverageRateOption`) is not a `Validatable`. Its
constructor throws `ConfigurationError` when an input cannot be priced. The
setters used while valuing a contract assert in Debug and do not re-check in
Release.

A function that used to return a value through `bool& successful` or
`std::string& errorMessage` returns `Result<T>` instead: the value, and the
`ValidationReport` for the inputs. `ok()` is false when that report has an
error.

## 4. No silent fallbacks

A function does not return `0`, `NaN`, or a sentinel date to mean "failed".
Where a fallback is genuinely wanted, for example flat extrapolation before
the first pillar, the function records a `WARNING` on the validation report
so the caller can see that it happened.

## Python

`sjd_module.cpp` registers a translator. `sjd::Error` becomes `sjd.SjdError`
(`RuntimeError`) and keeps `.code`. The three derived types keep the old
`except` clauses working:

| C++ | Python | Also a |
| --- | --- | --- |
| `ConfigurationError` | `sjd.ConfigurationError` | `ValueError` |
| `ValidationError` | `sjd.ValidationError`, with `.report` | `ValueError` |
| `NumericalError` | `sjd.NumericalError` | `ArithmeticError` |

`validate(env=None)` runs the checks and returns a `ValidationReport`.
`is_valid` and `report` only read the last report. `is_ok`, `error_messages`,
`validation_report` and `validate_report` remain as the old names.
`str(report)` is one `path: code: message` line per issue.
`sjd.require_valid(obj, env=None)` validates and raises `ValidationError`.
