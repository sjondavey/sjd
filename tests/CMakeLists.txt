# Boost.Test runner. Cases register themselves. Support headers are included
# as <support/...> from this directory.
add_executable(sjdTest
    contracts/AsianOptionTest.cpp
    contracts/CashflowTest.cpp
    contracts/ForwardContractTest.cpp
    contracts/OptionContractTest.cpp
    contracts/Portfolio/CVA/CVATest.cpp
    contracts/Portfolio/Insurance/PortfolioInsuranceTest.cpp
    contracts/Portfolio/Margin/MarginTest.cpp
    contracts/Portfolio/PortfolioEvolverTest.cpp
    contracts/Portfolio/PortfolioTest.cpp
    contracts/Portfolio/Structures/StructureTest.cpp
    maths/MathsTest.cpp
    maths/TwoDimensionalInterpolationTest.cpp
    pricing/AsianOptionFormulaTest.cpp
    pricing/Black76FormulaTest.cpp
    rates/Discount/DiscountCurvesTest.cpp
    rates/Discount/InterestRateConventionTest.cpp
    rates/Fixing/FixingRatesTest.cpp
    rates/Forward/CalendarMonthAverageTest.cpp
    rates/Forward/ForwardCurveFXTest.cpp
    rates/Forward/ForwardCurveTest.cpp
    rates/Volatility/VolatilityCurveTest.cpp
    rates/Volatility/VolatilitySurfacesTest.cpp
    golden/GoldenValuesTest.cpp
    tools/ErrorsTest.cpp
    support/GenerateContracts.cpp
    support/GenerateRatesEnvironments.cpp
    sjdTest.cpp
)

target_include_directories(sjdTest
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(sjdTest
    PRIVATE
        sjd::sjd
        Boost::unit_test_framework
        sjd_warnings
)

add_test(
    NAME sjdTest
    COMMAND sjdTest --log_level=message --build_info=yes --report_level=short
)

# Measurements, not a ctest gate. The 10,000-path case is one repetition.
find_package(benchmark CONFIG REQUIRED)
add_executable(sjdBenchmark
    benchmarks/Benchmarks.cpp
    support/GenerateContracts.cpp
    support/GenerateRatesEnvironments.cpp
)
target_include_directories(sjdBenchmark
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(sjdBenchmark
    PRIVATE
        sjd::sjd
        benchmark::benchmark
        sjd_warnings
)
