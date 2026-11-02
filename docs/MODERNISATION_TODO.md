# sjd modernisation plan

This is a to-do list for modernising the `sjd` C++ engine and finishing the move
from Excel to Python. Each step says what to do, why, which files it touches,
and how to tell when it is done. Nothing here has been executed yet.

The early steps are detailed because they are mechanical and low-risk, and they
build the safety net the later steps rely on. The later steps are deliberately
coarser; expand each one into its own sub-plan when you reach it.

Work the steps in order unless a step says otherwise. Each step should land as
one or more small commits with the full test suite green.

---

## Where the code is today

A snapshot taken at the start of the plan, so later steps can refer back to it.

**Shape.** About 16k lines of C++ in `include/sjd` and `src` (contracts, rates,
pricing, maths, tools), a Boost.Test suite in `tests/`, a 1,200-line pybind11
module in `python/sjd_module.cpp`, and a Python package in `python/sjd`
(`stats`, `strategy`, `optimise`, `markets`, `examples`). The Excel add-in and
ObjectHandler layers have been removed. Statistics (EE, PFE, CVA, DVA, funding,
CEM capital along a path) have already moved to Python; simulation stays in C++.

**Uncommitted work in progress.** `PortfolioEvolver::valueSpotPaths`, its
binding, `sjd.strategy`, `sjd.optimise`, `stats.structure_costs`, and their
tests.

**Error handling uses five different styles at once:**

| Style | Where |
| --- | --- |
| `Validatable` / `IssueTracker` / `ValidationReport` with issue codes and paths | Contracts, rate sources, portfolio, evolver, margin, CVA, insurance |
| A private `errorMessage` string plus `isOK()` / `isOk()` | `Black76Option`, `AverageRateOption`, `ArrayInterpolator`, `Curve`, `DatedCurve`, `CalendarMonthAverageCurve` |
| `bool&` / `std::string&` output parameters | `Structure::createStructures`, `CurrentExposureMethod::optimiseStructuresFromContracts`, `nettingSets(std::string&)` |
| Exceptions | `makeForwardCurve`, `calendarNamed`, and the pybind layer |
| Silently returning `0`, `NaN`, or a sentinel date | `VolatilitySurfaceDelta` (catches all exceptions and returns 0), `CurrentExposureMethod` (NaN), `Structure::getSettlementDate` (1 Jan 1910) |

The `Validatable` design is good and is what should be kept and extended. Even
there, `isOK()` has two meanings: on contracts it only reads the last result,
while on everything else it clears and re-runs every check (the comment in
`include/sjd/tools/Validation.h` admits this).

**No logging.** Nothing in the library logs. The only diagnostics are
validation messages and the debug-CRT redirect in `sjd_module.cpp`.

**C++ idiom.** C++17, but mostly written in C++03 style: 151 raw `new`
expressions and no `make_shared` / `make_unique`; 429 `virtual` against 33
`override`; mixed `#ifndef` guards and `#pragma once` (some commented out);
mixed tabs and spaces; vectors passed by value; getters that should be `const`
are not, because `getValue` writes a cache.

**Latent bugs and portability problems found while reading:**

- `Structure::qualifyForRegulatoryCapitalOffset(..., std::string &reasonIfNot = std::string(""))`
  binds a non-const reference to a temporary. Only MSVC accepts this.
- `generateRegCapFactors` returns a null pointer for an unrecognised string.
- `SingleFactorEconomicScenarioGenerator` seeds its RNG with `time(0)`, so
  unseeded runs cannot be reproduced, and the `seed` member is never
  initialised.
- The generator hands back the same mutable `Path` object on every call, so
  paths are not independent objects and the engine cannot run in parallel.
- `VolatilitySurfaceDelta` swallows solver and interpolation exceptions and
  returns a strike or delta of 0, which then gets priced.
- `CumulativeDefaultProbability` declares its own `anchorDate`, shadowing the
  one in `CVA`.
- `Portfolio()` (the default constructor) leaves `hasMargin` uninitialised.
  The margin constructors set `hasMargin = true` even when the margin pointer
  is null.
- `PortfolioEvolver::isOK()` has side effects: it sets the generator's dates
  and values the whole book along a 95% path. `simulatePaths` calls it and
  then sets the dates a second time.
- `AsianContract::getAllBusinessDaysBetween` computes
  `lastDateInAveragingMonth` and never uses it.
- Typos in the public API: `getForwardFworwardAdditive`,
  `setVolitalityRateSource`, `updateAppedededRates`, `isContactADerivative`,
  `hasPriceExpsoure`, and test suites `ForwardContactTest` / `OptionContactTest`.
- Comments contain `?` where a non-ASCII character was lost in an encoding
  conversion (for example `Structure.h`, `Margin.h`, `RateSource.h`).

**Build and tooling.** CMake presets with vcpkg, Windows only in practice.
`CMakeLists.txt` hard-codes `python310.lib` and finds pybind11 by running
Python. The compiled module is copied into the source tree. No CI, no
`.clang-format`, `.clang-tidy`, or `.editorconfig`. Git warns about LF/CRLF on
several files. Python tests are `unittest` scripts that `ctest` runs one by one.

**Python bindings are missing:** `Cashflow`, `ContractStressor`,
`PortfolioInsurance` (`SimpleInsurance`, `SimpleInsuranceWithTermStructure`),
`RatesEnvironment.setForeignDRS` (so FX forwards cannot be built from Python),
`VolatilitySurfaceMoneyness` (non-CMA), `AverageRateOption`, and the full
`Margin::setMarginParameters` (initial margin and independent amounts).

---

## Guiding rules for every step

1. **Numbers must not move by accident.** Step 3 creates golden-value tests. Any
   step that changes a golden value must say so in its commit message and
   explain why.
2. **Keep the Python API stable, or deprecate it first.** When a binding name
   changes, keep the old name for one release with a `DeprecationWarning`.
3. **Validation stays out of the Monte Carlo hot path.** The engine checks once
   at setup and then assumes inputs are valid. Keep that contract.
4. **One concern per commit.** Mechanical changes (formatting, renames) never
   share a commit with behaviour changes.

---

## Phase 0 — Baseline and safety net

### Step 0. Land the in-progress work and tag a baseline

**Goal.** Start the refactor from a clean, tagged commit.

**Tasks**

- [ ] Finish and commit the uncommitted work: `PortfolioEvolver::valueSpotPaths`
      and its binding, `python/sjd/strategy.py`, `python/sjd/optimise.py`,
      `stats.structure_costs`, `python/tests/test_strategy.py`,
      `python/tests/test_optimise.py`, README updates.
- [ ] Decide whether `.vscode/c_cpp_properties.json` belongs in the repo. If it
      does not, add `.vscode/` (except perhaps `settings.json`) to `.gitignore`.
- [ ] Run `ctest --preset debug` and record the result.
- [ ] Tag the commit `pre-modernisation`.

**Done when** `git status` is clean, all tests pass, and the tag exists.

---

### Step 1. Build hygiene and tooling

**Goal.** A reproducible, portable build with consistent formatting and
warnings switched on.

**Why.** The build only works on Windows with Python 3.10 installed in a
particular way. There is no automated formatting, and warnings are at `/W3`.

**Tasks — CMake**

- [ ] Replace the hand-rolled Python discovery in `CMakeLists.txt` with
      `find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter Development.Module)`
      and `find_package(pybind11 CONFIG REQUIRED)` (add `pybind11` to
      `vcpkg.json`, or keep pip's pybind11 and pass `pybind11_DIR`). Build the
      module with `pybind11_add_module(sjd_python MODULE python/sjd_module.cpp)`.
      This removes the hard-coded `python310.lib` and the `/NODEFAULTLIB` hack.
- [ ] Use `${PROJECT_SOURCE_DIR}` instead of `${CMAKE_SOURCE_DIR}` in
      `src/CMakeLists.txt` so `sjd` can be consumed with `add_subdirectory` or
      `FetchContent`.
- [ ] Add a `BUILD_INTERFACE` / `INSTALL_INTERFACE` include split and
      `install(TARGETS sjd EXPORT sjdTargets ...)` plus a package config file,
      so other C++ projects can `find_package(sjd)`.
- [ ] Add a `release` test preset and a `relwithdebinfo` configure preset to
      `CMakePresets.json`. Add a Linux preset (`x64-linux` triplet, GCC or
      Clang) even before CI exists.
- [ ] Add an `SJD_WARNINGS_AS_ERRORS` option (default `OFF` locally, `ON` in
      CI). Raise warnings to `/W4 /permissive-` on MSVC and
      `-Wall -Wextra -Wpedantic` on GCC/Clang. Expect a large first batch of
      warnings; fix them in Step 4 and Step 5, not here.
- [ ] Prune `vcpkg.json`: check whether `boost-assign`, `boost-smart-ptr` and
      `boost-algorithm` are still needed after Step 5. `boost-timer` is only
      used by the test runner and goes away in Step 17.

**Tasks — repository configuration**

- [ ] Add `.clang-format` (suggest: based on `Microsoft` or `LLVM`, 4-space
      indent, 110-column limit, braces on their own line to match the current
      style). Do **not** reformat yet; that is Step 4.
- [ ] Add `.clang-tidy` with a modest set: `modernize-use-override`,
      `modernize-make-shared`, `modernize-make-unique`, `modernize-use-nullptr`,
      `modernize-use-equals-default`, `performance-unnecessary-value-param`,
      `readability-const-return-type`, `bugprone-*`, `cppcoreguidelines-init-variables`.
- [ ] Add `.editorconfig` (UTF-8, spaces, final newline, CRLF or LF —
      pick one).
- [ ] Extend `.gitattributes` so line endings are normalised
      (`* text=auto eol=lf` or `eol=crlf`) and renormalise once
      (`git add --renormalize .`) in its own commit.
- [ ] Add `ruff` configuration to `python/pyproject.toml` (lint and format).

**Done when** a fresh clone builds with `cmake --preset debug` on Windows
without Python 3.10 being special-cased, and `clang-format --dry-run` and
`ruff check` run (they are allowed to report problems at this stage).

---

### Step 2. Continuous integration

**Goal.** Every push builds and tests on Windows and Linux.

**Tasks**

- [ ] Add `.github/workflows/ci.yml` (the history shows the remote is
      `github.com/daveysj/sjd`). Matrix: `windows-latest` (MSVC) and
      `ubuntu-latest` (GCC). Debug and Release.
- [ ] Cache vcpkg packages with vcpkg binary caching (`VCPKG_BINARY_SOURCES`
      using the GitHub Actions cache); QuantLib is slow to build.
- [ ] Steps: configure, build, `ctest --output-on-failure`, `ruff check`,
      `clang-format --dry-run --Werror` (once Step 4 lands).
- [ ] Add a Linux job built with `-fsanitize=address,undefined` that runs
      `sjdTest`. This is the quickest way to flush out uninitialised members
      and out-of-range indexing.
- [ ] Turn on `SJD_WARNINGS_AS_ERRORS` in CI once Steps 4 and 5 have brought
      the warning count to zero.

**Done when** a pull request shows green checks on both platforms.

**Note.** The Linux build will fail until the non-const-reference default
argument in `Structure.h` is fixed. Fix that one line here (see Step 6) rather
than waiting.

---

### Step 3. Golden-value regression tests and pytest

**Goal.** Lock down today's numbers so refactors can be checked automatically.

**Why.** Steps 4 to 16 change a lot of code. The existing tests check
properties, but not that a given book still gives exactly the same price,
exposure and capital.

**Tasks**

- [ ] Create `python/tests/golden/` with a script `make_golden.py` that builds
      the sample market (`sjd.examples.sample_market()`) and a fixed set of
      books, then writes a JSON file of results:
  - `get_value` for every contract type (European forward and option, Asian
    forward and option, cashflow once bound) across several strikes and dates;
  - Black76 and Asian formula premiums and deltas on a grid of inputs;
  - forward, discount and volatility lookups for each curve type;
  - a seeded simulation (`seed=42`, 2,000 paths, antithetic) of a margined and
    an unmargined book: mean MtM, EE, PFE at each date, and the first three
    paths in full;
  - `stats.regulatory_capital` and `PortfolioB2CEM.get_regulatory_capital` for
    the sample books, with and without `optimise`, and the netting-set
    descriptions.
- [ ] Add `python/tests/test_golden.py` that recomputes everything and compares
      with `numpy.testing.assert_allclose` (relative tolerance around `1e-10`
      for closed-form values; exact for seeded paths on the same platform,
      looser across platforms).
- [ ] Switch the Python test runner to `pytest` (it runs the existing
      `unittest` classes unchanged). Add `[tool.pytest.ini_options]` to
      `pyproject.toml`. Replace the per-file `add_test` loop in
      `CMakeLists.txt` with one `add_test(NAME sjdPython COMMAND python -m pytest python/tests)`,
      or keep per-file tests via `pytest path::` if you prefer separate ctest
      entries.
- [ ] Record C++-only golden values (things not reachable from Python yet, such
      as `ContractStressor` and `PortfolioInsurance`) in a Boost test case that
      compares against hard-coded numbers.

**Done when** `ctest` runs the golden tests, and any change to a priced value
larger than the 1e-9 relative tolerance makes a golden test fail.

**Status (done).** Python golden values live in `python/tests/golden/golden.json`
and are regenerated with `python python/tests/golden_cases.py`. The C++ tables
in `tests/golden/GoldenValuesTest.cpp` are reprinted by running `sjdTest` with
`SJD_PRINT_GOLDEN=1` and pasting the output. Regenerate only when a change is
meant to move numbers, and explain each move in the commit.

---

## Phase 1 — C++ hygiene

**Status.** Steps 4 and 6 are done. Step 5 is done for the edits that do not
need clang-tidy: scoped enums, `make_shared` / `make_unique`, `sign(BuySell)`,
`std::to_string`, `<random>`, default member initialisers for the known gaps,
and `const` on the named getters. The library stays on C++17; `std::format`
waits until Step 7. MSVC `/W4` reports nothing and the Windows CI jobs compile
with `SJD_WARNINGS_AS_ERRORS=ON`. clang-tidy has not been run, so `override`,
`= default`, `nullptr` and pass-by-`const`-reference are still open. Linux
`-Werror` stays off until a GCC build with `-Wall -Wextra -Wpedantic` is clean.

### Step 4. Mechanical clean-up

**Goal.** Make the code consistent so later diffs only show real changes.

**Tasks** (one commit each)

- [x] **Reformat** the whole C++ tree with `.clang-format`. Put the commit hash
      in `.git-blame-ignore-revs`.
- [x] **Header guards.** Use `#pragma once` everywhere and remove the
      `#ifndef ... #define ... #endif` blocks (or the reverse; just be
      consistent). Remove commented-out `//#pragma once` lines.
- [x] **Includes.** Remove heavy or unused includes from headers. Examples:
      `Margin.h` includes `<ql/time/calendars/all.hpp>` and
      `<boost/math/special_functions.hpp>`; `RegCapUnderlyingFactors.h`
      includes `<boost/algorithm/string.hpp>`; `Contracts.h` includes
      `<sstream>` and `<iomanip>`. Move them to the `.cpp` files that need
      them. Consider running include-what-you-use once.
- [x] **`using namespace boost;`** appears in most `.cpp` files and is almost
      never needed. Remove it; keep `using namespace std;` only if you must, or
      remove that too.
- [x] **Encoding.** Convert every source file to UTF-8 and fix the `?`
      characters in comments.
- [x] **Dead code.** Delete the commented-out block at the bottom of
      `Structure.h`, the commented includes in `maths.h`, the empty
      `configure()` in `tests/sjdTest.cpp`, and the unused
      `lastDateInAveragingMonth` in `Contracts.cpp`.
- [x] **Stale comments.** `maths.h` still says NaNs are returned "because the
      files are used in Excel". `Common.h` still describes the old design.
      Update or delete these.
- [x] **Typos in identifiers.** Rename `getForwardFworward*` to
      `getForwardForward*`, `setVolitalityRateSource` to
      `setVolatilityRateSource`, `updateAppedededRates` to
      `updateAppendedRates`, `isContactADerivative` to `isContractADerivative`,
      `hasPriceExpsoure` to `hasPriceExposure`, `maxItterates` to
      `maxIterations`, `ForwardContactTest` to `ForwardContractTest`,
      `OptionContactTest` to `OptionContractTest`. None of these are bound in
      Python today (check `sjd_module.cpp` before each rename).

**Done when** the golden tests still pass bit-for-bit and `clang-format
--dry-run --Werror` is clean. Enable the formatting check in CI.

---

### Step 5. Modern C++ idioms

**Goal.** Bring the code up to idiomatic C++17 (or C++20, see the note) without
changing behaviour.

**Tasks**

- [ ] **`override` and `= default`.** Run `clang-tidy -fix` with
      `modernize-use-override` and `modernize-use-equals-default`. Every
      overriding function gets `override`; empty virtual destructors become
      `~X() override = default;` (or just `virtual ~Base() = default;` on base
      classes). Not done: clang-tidy has not been run.
- [x] **Ownership.** Replace `std::shared_ptr<T>(new T(...))` with
      `std::make_shared<T>(...)` and `std::unique_ptr<Contract>(new X(*this))`
      with `std::make_unique<X>(*this)`. Raw `T*` allocations that are later
      `delete`d were left as they are.
- [x] **Initialise every member.** The known gaps are initialised:
      `Portfolio::hasMargin`, `SingleFactorEconomicScenarioGenerator::seed`,
      and the primitive members of `SingleInstrumentContract`. A whole-tree
      audit is still open. The sanitizer job from Step 2 confirms the fix.
- [ ] **Parameter passing.** Pass `std::vector`, `std::set`, `std::string` and
      `std::shared_ptr` by `const&` unless the function stores a copy (then
      pass by value and `std::move`). Not done.
- [x] **`const` correctness.** `getAnchorDate`, `getMarginBalance` and
      `isMargined` are `const`. `getVolume` and `getStrike` already were.
      `getValue` stays non-const until Step 13. Other getters were left alone.
- [ ] **`nullptr`.** Replace comparisons such as `if (re == 0)` and
      `dynamic_pointer_cast<...>(x) != 0` with `nullptr` or a plain boolean
      test. Not done: many `== 0` comparisons are numeric, so this needs
      clang-tidy rather than a text replace.
- [x] **Scoped enums.** `BuySell`, `PutCall` and `ArrayInterpolatorType` are
      `enum class`. `ForwardAveraging` and `ForwardRoll` already were. The
      pybind names are unchanged. `buySellAsDouble` is replaced by `sign(BuySell)`.
- [x] **Replace `boost::lexical_cast<std::string>(i)`** with `std::to_string`,
      and `boost::random` with `<random>` (`std::mt19937`,
      `std::normal_distribution`). `boost::math::normal` still supplies the
      confidence-interval quantile. The RNG commit regenerates only the
      simulation golden values.
- [x] **C++20.** Decided: stay on C++17. `std::format` waits until Step 7.

**Done when** clang-tidy with the Step 1 configuration reports nothing, `/W4`
and `-Wall -Wextra` are zero, and the golden tests pass. `/W4` is zero and the
golden tests pass. Windows CI compiles with `SJD_WARNINGS_AS_ERRORS=ON`.
Linux `-Werror` stays off until `-Wpedantic` is clean. clang-tidy has not been
run, so this step is not fully done.

---

### Step 6. Fix the latent bugs

**Goal.** Fix the defects found while reading the code, each with a test that
failed before the fix.

| Defect | File | Fix |
| --- | --- | --- |
| ~~Non-const reference default argument~~ (fixed in Step 1 with an overload, because `/permissive-` rejects it) | `Structure.h` (`qualifyForRegulatoryCapitalOffset`) | Optionally return `std::optional<std::string>` instead (the reason, or empty when it qualifies) |
| Standard deviation interpolated linearly from 0 at the anchor, so a flat 12% curve gives about 8.4% between the anchor and the first pillar | `VolatilityCurve.cpp` | Interpolate total variance (σ²t) instead, so that a flat curve stays flat; update the golden values that move |
| Linear extrapolation before the first pillar gives a negative PD at the anchor (about −0.42% on the sample curve), and an implied LGD outside the input range | `cva.h` / `cva.cpp` (`CumulativeDefaultProbability`) | Pin PD to 0 at the anchor and interpolate from there, or flat-extrapolate; reject inputs that are not non-decreasing |
| Unknown asset class gives a null factor table | `RegCapUnderlyingFactors.cpp` | Throw `sjd::ConfigurationError` (Step 7) listing the accepted names; the Python `reg_cap_factors` already raises, so align the messages |
| Exceptions swallowed and 0 returned | `VolatilitySurfaceDelta.cpp` (`calculateDeltaFromStrike`, `calculateStrikeFromDelta`) | Propagate a `NumericalError`, or record a validation issue at setup if this can be detected once; never price with a zero strike |
| Time-based RNG seed; `seed` never initialised | `SingleFactorGMB.cpp` | Default seed from `std::random_device`, stored in `seed` and reported back to the caller (Step 12) |
| `anchorDate` shadowed | `cva.h` (`CumulativeDefaultProbability`) | Delete the duplicate member |
| Margin constructors accept a null margin but set `hasMargin = true` | `Portfolio.cpp` | Derive `hasMargin` from the pointer, or reject null in validation |
| `isOK()` with side effects; dates set twice | `PortfolioEvolver.cpp` | Split into a pure `validate()` and a `prepare()` step (full fix in Step 8) |
| Sentinel date 1 Jan 1910 for an empty structure | `Structure.cpp` (`getSettlementDate`) | Return `std::optional<Date>` or make an empty structure invalid at construction |
| `PORTFOLIO_EMPTY` message begins with `", "` | `Contracts.cpp` | Fix the text |

**Done when** each fix has its own test and the golden tests pass, apart from
any value that was wrong because of the bug (explain each one in the commit).
Done. `ConfigurationError` and `NumericalError` already live in
`include/sjd/tools/Errors.h`; Step 7 adds the rest of that hierarchy.

---

## Phase 2 — Error handling and reporting

### Step 7. Write down an error-handling policy and add the exception types

**Goal.** One rule for every kind of failure, so each class knows which
mechanism to use.

**Proposed policy** (record it in `docs/error-handling.md`):

1. **Bad inputs that can be found before calculating** — missing curves, dates
   out of range, negative strike, structures that cannot be netted — are
   *validation issues*. They are recorded in the object's `ValidationReport`
   with a stable code, a message, a severity, and a path. This is the existing
   `Validatable` design and it stays.
2. **Problems that make an object impossible to construct**, such as an
   unknown calendar or asset-class name, or a curve with mismatched vector
   lengths, *throw*. Use a small hierarchy in `include/sjd/tools/Errors.h`:
   - `sjd::Error : std::runtime_error` with a `code()` string;
   - `sjd::ConfigurationError : Error` for bad names and options;
   - `sjd::ValidationError : Error`, which carries a `ValidationReport` and is
     thrown by convenience functions that validate and then calculate;
   - `sjd::NumericalError : Error` for solver non-convergence and similar.
3. **Programming errors and preconditions inside the hot path** — an index out
   of range, or `getValue` called without validation — are *assertions*
   (`SJD_ASSERT`, active in Debug, compiled out in Release). They are not
   checked at run time in Release, so Monte Carlo speed is preserved.
4. **No silent fallbacks.** Never return `0`, `NaN` or a sentinel to mean
   "failed". If a fallback is genuinely wanted (for example flat
   extrapolation), record a `WARNING` issue so the caller can see it.

**Tasks**

- [ ] Add `src/tools/Errors.cpp`, `ValidationError`, and the `SJD_ASSERT` /
      `SJD_REQUIRE` macros. `include/sjd/tools/Errors.h` already has `Error`,
      `ConfigurationError` and `NumericalError` from Step 6. `SJD_REQUIRE`
      throws in every build; `SJD_ASSERT` is debug-only.
- [ ] Convert the existing `throw std::invalid_argument` sites
      (`ForwardCurveFactory.cpp`, `Calendars.cpp`) to `ConfigurationError`.
      Since `ConfigurationError` derives from `std::runtime_error` and not
      `std::invalid_argument`, update the `catch` in `sjd_module.cpp` at the
      same time (Step 10 replaces it with a translator).
- [ ] Add tests for each exception type.

**Done when** the policy document exists and the two existing throw sites use
the new types.

---

### Step 8. Unify the validation API

**Goal.** Every `Validatable` behaves the same way, and `isOK()` never has side
effects.

**Tasks**

- [ ] Give `Validatable` one public entry point:
      `ValidationReport validate()` (and, for contracts and anything else that
      depends on a market, `ValidationReport validate(const RatesEnvironment&)`).
      It clears the tracker, runs every check, and returns the report.
- [ ] Make `isOK()` a non-virtual, `const` read of the last report, and rename
      it `isValid()` if you want to make the change in meaning obvious. Keep
      `isOK()` as a deprecated alias for one release.
- [ ] Remove the "everything else re-runs on `isOK()`" style from rate sources,
      generators, `PortfolioEvolver`, `Margin`, `CVA`, `PortfolioInsurance`,
      `ContractStressor` and `Path`. Callers that relied on `isOK()` re-running
      must now call `validate()`.
- [ ] Actually use severities. `IssueTracker::record` always writes `ERROR`; add
      `recordWarning` / `recordInfo` and use them for things like extrapolation
      past the last pillar and flat back-fill before the first observation.
- [ ] Add to `ValidationReport`: `merge`, `errors()`, `warnings()`, a readable
      `toString()` (one line per issue, `path: code: message`), and `toJson()`.
- [ ] Replace `getErrorMessages()` / `getErrorMessagesAsString()` with
      `validationReport().toString()`. Keep the old functions as deprecated
      wrappers until the bindings are updated (Step 10).
- [ ] Remove the duplicate `className` members in `Margin`, `CVA` and
      `PortfolioInsurance`; the tracker already holds the class name.
- [ ] Collect every issue code in one place — either an `enum class IssueCode`
      with a `toString`, or `constexpr std::string_view` constants in
      `include/sjd/tools/IssueCodes.h` — and generate a table of codes and
      meanings for the docs. Today the codes are string literals scattered
      across about 34 files.
- [ ] Make `PortfolioEvolver` split cleanly into `validate()` (no side effects),
      `prepare()` (sets generator dates and builds the path template once), and
      `simulatePaths()` (asserts that `prepare()` succeeded).

**Done when** no `isOK()` implementation modifies state, every validatable
class exposes `validate()` returning a report, and the Python tests pass
through the deprecated aliases.

---

### Step 9. Move the other error styles onto the policy

**Goal.** Remove the private `errorMessage` strings, the output parameters and
the silent fallbacks.

**Tasks**

- [ ] **Pricing formulas** (`Black76Option`, `AverageRateOption`). These are on
      the hot path. Either (a) validate inputs once in a constructor and throw
      `ConfigurationError`, with `SJD_ASSERT` in the setters used by Monte
      Carlo, or (b) make them `Validatable` like the rest. Option (a) suits a
      formula better. Remove `getErrorMessage()`.
- [ ] **Interpolators and curves** (`ArrayInterpolator`,
      `TwoDimensionalInterpolation`, `Curve`, `DatedCurve`,
      `CalendarMonthAverageCurve`). These are built once from market data, so
      they should be `Validatable` and report through the owning rate source's
      report with a path such as `forward_curve/interpolator`.
- [ ] **Output parameters.** Replace `bool& successful, std::string& errorMessage`
      in `Structure::createStructures`, `decomposeIntoCappedAndUncapped`,
      `optimiseCashflow` and `CurrentExposureMethod::optimiseStructuresFromContracts`
      with a return type such as:

      ```cpp
      template <class T>
      struct Result {
          T value;
          ValidationReport report;
          bool ok() const { return report.isOK(); }
      };
      ```

      (or `tl::expected` from vcpkg, or `std::expected` if you move to C++23).
      Replace `nettingSets(std::string& error)` the same way.
- [ ] **NaN returns** in `CurrentExposureMethod::getRegulatoryCapital` and
      `getNGR`: replace with `SJD_ASSERT(!hasErrors())`. Callers must validate
      first, as with contracts.

**Done when** `rg "errorMessage|bool ?& ?successful|quiet_NaN" src include`
finds nothing that signals failure, and the golden tests pass.

---

### Step 10. Python error surface

**Goal.** Python users see proper exception types and readable reports.

**Tasks**

- [ ] Register pybind11 exception translators in `sjd_module.cpp`: `sjd::Error`
      becomes `sjd.SjdError`, `ConfigurationError` becomes
      `sjd.ConfigurationError(SjdError, ValueError)`, `ValidationError` becomes
      `sjd.ValidationError(SjdError, ValueError)` with a `.report` attribute,
      and `NumericalError` becomes `sjd.NumericalError(SjdError, ArithmeticError)`.
      Subclassing `ValueError` keeps existing `except ValueError` code working.
- [ ] Replace the `throw py::value_error("Cannot simulate: " + ...)` patterns
      with `throw sjd::ValidationError(report)` so Python gets the structured
      report, not just a string.
- [ ] Settle on one naming scheme for the bound validation methods. Today the
      bindings mix `is_ok`, `error_messages`, `validation_report`,
      `validate_report` and `validate`. Suggested: `validate(env=None) ->
      ValidationReport`, `is_valid` (property), and `report` (property, the
      last report). Keep the old names as deprecated aliases.
- [ ] Give `ValidationReport` and `ValidationIssue` a `__str__`, `__repr__`, a
      `to_dict()` and `_repr_html_` (for a table in notebooks), and a
      `raise_if_invalid()` method.
- [ ] Add `sjd.require_valid(obj, env=None)`, which validates and raises
      `ValidationError`, and use it in `optimise.value_on_spots` and anywhere
      else that hand-builds error strings today.

**Done when** `pytest` covers each exception type and `str(report)` gives one
readable line per issue.

---

## Phase 3 — Logging and diagnostics

### Step 11. Add logging

**Goal.** Structured, levelled logging in C++ that shows up in Python's
`logging` module, with no cost in the Monte Carlo inner loop.

**Design**

- Use **spdlog** (available in vcpkg; built on `fmt`) behind a thin facade, so
  the backend can be swapped and so headers do not pull in spdlog:
  `include/sjd/tools/Log.h` declares `sjd::log::logger()` and the macros
  `SJD_LOG_TRACE`, `SJD_LOG_DEBUG`, `SJD_LOG_INFO`, `SJD_LOG_WARN`,
  `SJD_LOG_ERROR`. Define `SPDLOG_ACTIVE_LEVEL` so `TRACE` and `DEBUG` compile
  out of Release builds.
- Default sink: nothing (a library should be quiet unless asked). Tools and
  tests can attach a stderr sink.
- **Python bridge.** A custom spdlog sink forwards records to the Python logger
  `sjd._core`. The simulation releases the GIL (`py::gil_scoped_release` in
  `simulate` and `value_spot_paths`), so the sink must not call Python
  directly from worker code. Two options:
  1. buffer records in a lock-protected queue and flush them to Python after
     the C++ call returns (simplest; recommended); or
  2. take the GIL inside the sink (`py::gil_scoped_acquire`), accepting the
     contention, only for `WARN` and above.
- Expose `sjd.set_log_level(level)`, which sets both the C++ level and the
  Python logger level.

**What to log**

| Level | Event |
| --- | --- |
| INFO | Simulation start and end: paths, steps, trades, antithetic, seed, elapsed time |
| INFO | CEM structure optimisation result: number of netting sets, capped/uncapped split, effective volumes |
| DEBUG | Each netting-set decision in `Structure::createStructures` and `decomposeIntoCappedAndUncapped` (this is what explains a capital number) |
| DEBUG | Validation failures (summary of the report) when a top-level call is rejected |
| WARN | Solver non-convergence, extrapolation past the last pillar, flat back-fill to the anchor date |
| TRACE | Per-path values — compiled out of Release |

Never log inside the per-path, per-step loops above `TRACE`.

**Tasks**

- [ ] Add `spdlog` to `vcpkg.json`, write the facade and the Python sink, and
      bind `set_log_level`.
- [ ] Add the log statements in the table above.
- [ ] In the Python package, use `logger = logging.getLogger(__name__)` in
      `stats`, `optimise`, `strategy` and `markets`, and log at INFO for each
      optimiser trial and each market load.
- [ ] Test: capture logs with `caplog` in pytest and check that a simulation
      writes its start and end records and that a non-converging delta solve
      writes a warning.
- [ ] Benchmark a 10,000-path simulation before and after; the difference must
      be within noise.

**Done when** `logging.basicConfig(level=logging.INFO)` in a notebook shows the
simulation and structuring messages, and Release timings are unchanged.

---

### Step 12. Run provenance and reproducibility

**Goal.** Every result can be reproduced and traced back to the code and inputs
that produced it.

**Tasks**

- [ ] When `seed` is `None`, draw one from `std::random_device`, use it, and
      return it on the `Simulation` object (`simulation.seed`).
- [ ] Add `simulation.metadata`: library version, git commit, build type,
      timings for setup and paths, the warnings from the validation report,
      and the input parameters.
- [ ] Add `sjd.__version__` (single-sourced, see Step 18) and
      `sjd.build_info()` returning compiler, build type, QuantLib and Boost
      versions, and the git commit (passed in by CMake at configure time).
- [ ] Give `stats.summary` and `stats.structure_costs` an option to include the
      metadata in their output.

**Done when** two runs with the reported seed give identical arrays, and a
notebook can print where a number came from.

---

## Phase 4 — Architecture

These steps change structure, not just style. Each needs its own short design
note before coding.

### Step 13. Immutable contracts and explicit caching

**Problem.** `Contract::getValue` writes a cache keyed on the
`RatesEnvironment` pointer and its market revision. This makes `getValue`
non-const and contracts unsafe to share between threads, and the cache misses
edits made inside a curve that is already attached (the header documents this).

**Direction.** Make contracts immutable once built: constructor arguments (or a
builder) instead of setters, and no per-contract cache. Where caching really
pays off (CEM capital calls `getValue` several times per environment), cache in
the caller — for example the evolver keeps a per-step vector of trade values
and passes it to capital. The `by_trade` output already contains these values.

**Done when** `getValue` is `const`, contracts have no mutable state, and the
simulation is no slower (benchmark).

---

### Step 14. Separate regulatory capital from the portfolio

**Problem.** `PortfolioB2CEM` is a `Portfolio` that owns a
`CurrentExposureMethod`. Capital is therefore tied to one portfolio class, and
adding SA-CCR would mean another portfolio subclass. The Basel II add-on table
is hard-coded as one C++ class per asset class, selected by string. The name
`Structure` means "CEM netting set" in C++ but the README now uses *strategy*
for user-facing books.

**Direction**

- A `CapitalModel` interface: `capital(const Portfolio&, const Margin*, const
  RatesEnvironment&) -> CapitalResult`, where `CapitalResult` holds the EAD,
  the RWA, the capital, and a breakdown by netting or hedging set.
  `CurrentExposureMethod` implements it; SA-CCR (Step 22) will be the second
  implementation.
- `Portfolio` knows nothing about capital. Retire `PortfolioB2CEM` (keep a thin
  deprecated Python shim that builds a `Portfolio` and a `CurrentExposureMethod`).
- Replace the per-asset-class factor classes with an `AssetClass` enum
  (`InterestRate`, `FX`, `Gold`, `Equity`, `PreciousMetal`, `Commodity`, and
  the SA-CCR commodity sub-classes `Electricity`, `OilGas`, `Metals`,
  `Agricultural`, `OtherCommodity`) and data tables, so both CEM and SA-CCR
  read supervisory parameters from one place.
- Replace `RegCapCreditParameters` with a plain struct (counterparty risk
  weight, capital ratio — 8% by default).
- Rename C++ `Structure` to `CemNettingSet` (and `by_structure` to
  `by_netting_set` in Python, with an alias), so *structure* and *strategy*
  stop meaning two things.

**Done when** CEM capital is computed through `CapitalModel`, the golden capital
numbers are unchanged, and adding a second model needs no change to
`Portfolio`.

---

### Step 15. Simulation engine: independent paths, explicit RNG, parallelism

**Problem.** The scenario generator owns one `Path` and rewrites it on every
call; contracts cache values (Step 13); the RNG is a member of the generator.
This prevents running paths in parallel, which is the largest available
speed-up for the optimiser.

**Direction**

- The generator becomes stateless apart from its precomputed path template.
  `generatePath(rng, out)` writes into a caller-owned buffer.
- RNG is an explicit object passed in. Each worker gets an independent stream
  (counter-based such as Philox, or `std::mt19937_64` with jump-ahead / seed
  sequence), so results do not depend on the number of threads.
- Parallelise `simulatePaths` and `valueSpotPaths` over paths (`std::thread`
  pool or OpenMP; TBB is also in vcpkg). Each worker owns its environments.
- Offer quasi-random numbers (QuantLib's Sobol plus Brownian bridge) as an
  option.
- `SimulatedPaths.byTrade` is `paths × steps × trades` doubles and can run to
  gigabytes. Add an option to stream per-step aggregates (EE, PFE, capital)
  instead of returning the full cube, and an option for `float32` output.
- Rename `SingleFactorGMB.h` (the file name says GMB where GBM is meant) and
  make "single factor" a property of the model rather than of the class name,
  so a multi-factor model can be added later.

**Done when** seeded results are identical for 1 and N threads, and a
10,000-path simulation scales close to linearly up to the core count.

---

### Step 16. Rates layer tidy-up

**Tasks (outline)**

- `RatesEnvironment`: drop the `frsSet` / `drsSet` / ... booleans and test the
  pointers. Accept `const std::shared_ptr<T>&`. Consider a builder that
  validates on `build()`.
- The forward-curve classes are a cross product of averaging (daily or CMA),
  roll (deterministic, stochastic forward vol, stochastic spot vol) and FX.
  `makeForwardCurve` already hides this. Move towards composition (curve
  shape + roll model + optional FX carry) behind the factory, then hide the
  concrete classes from the public headers.
- Hold `QuantLib::DayCounter` and `QuantLib::Calendar` by value; they are
  already handle types, so the `std::shared_ptr<DayCounter>` and
  `std::shared_ptr<Calendar>` wrappers add nothing.
- Evaluate replacing the in-house `LinearArrayInterpolator`,
  `CubicSplineInterpolator` and `TwoDimensionalInterpolation` with QuantLib's
  interpolation classes. Keep the `sjd` interface and let the golden tests
  decide; expect tiny differences from the cubic spline end conditions.
- Make the bump-and-stress interface (`IHaveMarketRateStresses`, the
  `parallelBump` methods) consistent across all rate sources, ready for Python
  scenario analysis and for SA-CCR sensitivity checks.

---

### Step 17. Modernise the C++ test suite

**Tasks (outline)**

- Replace manual suite registration (`init_unit_test_suite` and
  `test->add(X::suite())` in `tests/sjdTest.cpp`, plus a header per test file)
  with `BOOST_AUTO_TEST_SUITE` / `BOOST_AUTO_TEST_CASE` and fixtures. This
  removes the `.h` file for each test and the deprecated `boost::timer`.
  (Moving to Catch2 or GoogleTest is an alternative; only do it if there is
  another reason, since it means rewriting about 7,400 lines of tests.)
- Use `BOOST_TEST(a == b, tt::tolerance(1e-12))` in place of hand-rolled
  tolerance checks.
- Add coverage: `gcov`/`lcov` in the Linux CI job, OpenCppCoverage on Windows.
  Publish the report as a CI artefact.
- Add benchmarks (Google Benchmark from vcpkg) for Black76, the Asian formula,
  one simulation step, and a full 10,000-path book, so Steps 13 and 15 can be
  measured.

---

## Phase 5 — Finish the Python layer

### Step 18. Python packaging

**Tasks (outline)**

- Build the extension with **scikit-build-core** (it drives the existing CMake
  build) so `pip install .` and `pip install -e .` compile `_core` themselves.
  This replaces copying the `.pyd` into the source tree and the separate CMake
  step described in `pyproject.toml`.
- Single-source the version (CMake `project(VERSION ...)` feeding both
  `sjd.__version__` and the C++ `build_info`).
- Build wheels with **cibuildwheel** in CI for Windows and Linux (and macOS if
  wanted).
- Generate type stubs (`pybind11-stubgen` produces `sjd/_core.pyi`), ship
  `py.typed`, and run `mypy` on the Python package in CI.
- Move Python tests and tooling configuration (`pytest`, `ruff`, `mypy`) into
  `pyproject.toml`.
- Accept `numpy.datetime64` and `pandas.Timestamp` as dates in `to_ql_date`,
  not only objects with `year`, `month` and `day` attributes.

---

### Step 19. Bind the outstanding C++ functionality

**Tasks (outline)** — bind, test and document each:

- `Cashflow` contract.
- `RatesEnvironment.set_foreign_drs` / `foreign_drs`, plus the FX forward
  curves, so FX books can be built from Python.
- `ContractStressor` (returns a list of `(name, value)`; return a dict or a
  pandas-friendly structure).
- `PortfolioInsurance`: `SimpleInsurance` and `SimpleInsuranceWithTermStructure`.
- `VolatilitySurfaceMoneyness` (non-CMA) and any discount-curve or
  interest-rate-convention classes not yet bound.
- `AverageRateOption` (Asian formula) alongside the existing Black76 bindings.
- `Margin::setMarginParameters` in full (initial margin both ways, thresholds),
  not just the `SimpleMargin` constructor.
- Capital model results from Step 14 (`CapitalResult` with its breakdown).
- Pickle support (`py::pickle`) for contracts, curves and portfolios, so books
  can be sent to `multiprocessing` workers and saved.
- `__repr__` on every bound class.

---

### Step 20. Python front end in place of Excel

**Tasks (outline)**

- A trade and portfolio input format (TOML, JSON or CSV) with a schema checked
  by `dataclasses` or `pydantic`, loading into `sjd` contracts. Reuse the
  `markets` catalogue pattern already in `python/sjd/markets`.
- Report outputs as pandas DataFrames (prices, exposure profiles, costs,
  capital), with optional Excel export via `openpyxl` for users who still want
  spreadsheets.
- A command-line entry point, for example `python -m sjd run book.toml
  --market sample --paths 10000`, writing a report folder.
- Run the notebooks in CI (`nbmake` or `pytest --nbval-lax`) so they do not
  rot; `python/notebooks/_collar_smoke.py` already hints at this.
- Extend `sjd.strategy` beyond `CollarStrategy` (strips, seagulls, participating
  forwards) and generalise `optimise.minimise_collar_put_moneyness` into a
  multi-parameter optimiser (`scipy.optimize` over strategy parameters with
  common random numbers).

---

### Step 21. Documentation

**Tasks (outline)**

- Python API docs with Sphinx or MkDocs (`mkdocstrings`), built from the
  existing docstrings and the generated stubs.
- C++ API docs with Doxygen; the headers already use `/** ... */` blocks in
  many places.
- An architecture page: the contract hierarchy, the rates layer, the simulation
  flow, and where Python takes over.
- `docs/error-handling.md` (from Step 7) and a generated table of validation
  issue codes (from Step 8).
- A methodology page for CEM capital as implemented (netting-set decomposition,
  net-to-gross ratio, margin treatment), which Step 22 will extend with SA-CCR.

---

## Phase 6 — Basel III

### Step 22. SA-CCR exposure (Basel III counterparty credit risk)

**Goal.** Compute exposure at default under the standardised approach for
counterparty credit risk (BCBS 279, now Basel Framework CRE52) alongside CEM,
both today and along simulated paths, and use it as the capital term in the
strategy optimiser.

**Prerequisites.** Step 14 (`CapitalModel` and supervisory tables). Steps 13
and 15 make it fast enough to call inside the optimiser, but are not strictly
required for a first version.

**Method summary** — check each parameter against the CRE52 text when
implementing:

- \( \text{EAD} = \alpha \, (\text{RC} + \text{PFE}) \), with \( \alpha = 1.4 \).
- Replacement cost: unmargined \( \text{RC} = \max(V - C, 0) \); margined
  \( \text{RC} = \max(V - C,\ \text{TH} + \text{MTA} - \text{NICA},\ 0) \).
- \( \text{PFE} = \text{multiplier} \times \text{AddOn}^{\text{aggregate}} \), with
  \( \text{multiplier} = \min\!\left(1,\ 0.05 + 0.95 \exp\!\left(\frac{V - C}{2 \times 0.95 \times \text{AddOn}^{\text{aggregate}}}\right)\right) \).
- Trade level: adjusted notional (commodity: price × quantity; FX: the
  foreign-currency leg converted to the domestic currency; equity: price ×
  number of units), times supervisory delta, times maturity factor.
- **Supervisory delta**: \( +1 \) / \( -1 \) for linear trades. For options,
  with \( d = \frac{\ln(P/K) + \tfrac12 \sigma^2 T}{\sigma \sqrt{T}} \), using
  the *supervisory* volatility \( \sigma \): bought call \( +\Phi(d) \), sold
  call \( -\Phi(d) \), bought put \( -\Phi(-d) \), sold put \( +\Phi(-d) \). For
  Asian options use the last averaging date for \( T \).
- **Maturity factor**: unmargined \( \sqrt{\min(M, 1\text{y}) / 1\text{y}} \)
  with \( M \) floored at 10 business days; margined
  \( 1.5 \sqrt{\text{MPOR} / 1\text{y}} \), with MPOR at least 10 business days
  (longer for large or disputed netting sets).
- **Commodity hedging sets** (energy, metals, agricultural, other): add-on per
  commodity type = supervisory factor × effective notional, then
  \( \text{AddOn}_{HS} = \sqrt{\left(\rho \sum_k A_k\right)^2 + (1 - \rho^2) \sum_k A_k^2} \).
- **Supervisory parameters** (CRE52.72): electricity SF 40%, \( \rho \) 40%,
  \( \sigma \) 150%; oil/gas, metals, agricultural and other commodities SF 18%,
  \( \rho \) 40%, \( \sigma \) 70%; FX SF 4%, \( \sigma \) 15%; equity single
  name SF 32%, \( \rho \) 50%, \( \sigma \) 120%; equity index SF 20%,
  \( \rho \) 80%, \( \sigma \) 75%.

**Tasks (outline)**

- [ ] Expose trade descriptors from C++ to Python (instrument type, buy/sell,
      put/call, strike, volume, maturity, settlement, last averaging date,
      asset class) — the equivalent of `netting_sets()` for CEM.
- [ ] Write a pure-Python reference implementation, `sjd.capital.saccr`, that
      takes a `Simulation` run with `by_trade=True` plus the trade descriptors
      and returns RC, PFE, multiplier and EAD shaped `[paths, exposure dates]`,
      and a single value for today. Black-76 \( d_1 \) is already available
      from the pricing formulas, but note SA-CCR uses supervisory, not market,
      volatility.
- [ ] Test against the worked examples in the BCBS 279 annex (there is a
      commodity example) and against hand calculations for a single forward,
      a single option, and a collar.
- [ ] Add `capital_model="cem" | "saccr"` to `stats.structure_costs` and the
      optimiser, with SA-CCR as the default once tested. Keep CEM available for
      comparison, as the README already promises.
- [ ] If the Python version is too slow inside the optimiser, port it to C++ as
      a second `CapitalModel` implementation and keep the Python version as the
      test oracle.
- [ ] Add a notebook comparing CEM and SA-CCR for outright, collar and seagull
      strategies, showing why delta-aware capital changes the optimiser's
      choice.

---

### Step 23. Basel III capital: risk weights and CVA capital

**Tasks (outline)**

- Turn EAD into capital: counterparty risk weight (standardised approach,
  CRE20) times EAD gives RWA; capital is RWA times the capital ratio (8% plus
  buffers, configurable).
- CVA risk capital under the basic approach (BA-CVA, reduced version, MAR50):
  per counterparty \( \text{SCVA}_c = \frac{1}{\alpha} \text{RW}_c \sum_{NS} M_{NS} \, \text{EAD}_{NS} \, \text{DF}_{NS} \)
  with \( \text{DF} = \frac{1 - e^{-0.05 M}}{0.05 M} \), aggregated with
  \( \rho = 50\% \) and scaled by \( \text{DS}_{BA\text{-}CVA} = 0.65 \). Check
  these against the MAR50 text when implementing.
- Optional: the leverage-ratio exposure measure, which also uses SA-CCR.
- Feed total capital (counterparty plus CVA) into the KVA term of
  `structure_costs`.
- Document the methodology and its limits (single asset class, one netting set
  per portfolio, no SA-CVA, no IMM).

---

### Step 24. Decide the future of CEM

Once SA-CCR has been in use for a while, decide whether to keep CEM as a
comparison model, freeze it, or remove it. If it stays, keep its golden tests;
if it goes, remove `CemNettingSet` and its decomposition logic (about 2,000
lines including tests) in one commit.
