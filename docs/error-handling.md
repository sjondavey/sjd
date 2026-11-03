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

## 4. No silent fallbacks

A function does not return `0`, `NaN`, or a sentinel date to mean "failed".
Where a fallback is genuinely wanted, for example flat extrapolation before
the first pillar, the function records a `WARNING` on the validation report
so the caller can see that it happened.

## Python

Until Step 10 registers a translator, the bindings catch `ConfigurationError`
at the calendar constructor and at `ForwardCurve` and raise `ValueError` with
the same message. Any other `sjd::Error` that escapes a binding becomes a
Python `RuntimeError`, because these types derive from `std::runtime_error`
and not from `std::invalid_argument`.
