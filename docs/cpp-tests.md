# C++ tests

Step 17. The suite stays on Boost.Test. The classes, the timer, and the
hand-written registration go. Coverage is a report, not a gate. Benchmarks
are a separate program.

## Registration

Each test class is a list of static methods, registered by hand from
`init_unit_test_suite` and again from `Class::suite()`. The class holds no
data.

`tests/sjdTest.cpp` defines `BOOST_TEST_MODULE` and `BOOST_TEST_MAIN`. That is
the one `init_unit_test_suite`. The library's `main` calls it, and it returns
an empty suite. Cases register themselves.

A class becomes `BOOST_AUTO_TEST_SUITE(ClassName)` in its `.cpp`. Each method
that `suite()` actually adds becomes `BOOST_AUTO_TEST_CASE` of the same name.
A method that is only mentioned in a comment stays out of the suite. Helper
methods become ordinary functions in that file. The per-test headers go away.
`boost::timer` goes away with them, and so does the `boost-timer` dependency.

There is no fixture type. Nothing in these classes is set up or torn down per
case. Setup that several cases share is already a function (`GenerateRates`,
`GenerateContracts`, or a helper in the `.cpp`).

The suite name is the class name, so `--run_test=CashflowTest` selects that
file's cases. The old names had spaces and did not match the class.

## Comparisons

`EPSILON_FOR_DOUBLES` is an absolute `1e-14`. Checks also use `10 *`, `100 *`,
and `volume *` that constant, and some residuals are near zero.

`tt::tolerance` is relative. `BOOST_TEST(a == b, tt::tolerance(1e-12))` would
accept large values the absolute check rejects, and reject a near-zero
residual the absolute check accepts. The absolute check stays, written as
`BOOST_CHECK_SMALL` (or `BOOST_REQUIRE_SMALL`) with the same threshold.
`BOOST_CHECK_SMALL(a - b, tolerance)` passes when `abs(a - b) < tolerance`.

A check that the values must differ stays a `BOOST_CHECK`. The golden tables
keep their relative `1e-9` comparison. That one already names its scale.

## Coverage

`SJD_COVERAGE` is off unless the configure line asks for it. It adds
`--coverage` to the library, the C++ tests, and the Python extension. It is
gcc/clang only, and it is not combined with the sanitizers. CI turns it on
for the Linux debug job, runs `ctest`, and uploads the `lcov` HTML. Both the
C++ runner and pytest contribute, because both link the instrumented library.

Windows debug CI runs `sjdTest` under OpenCppCoverage and uploads that HTML.
The report is the C++ runner only. There is no minimum percentage.

## Benchmarks

`sjdBenchmark` links Google Benchmark and is not a `ctest` test. It times a
Black76 premium, an Asian premium (a new `AverageRateCall` each iteration, so
the formula is not a cached read), one `simulatePaths(1)` on a one-option
book, and one `simulatePaths(10000)` on that book. The 10,000-path case runs
once. These numbers are measurements. They do not fail the build.
