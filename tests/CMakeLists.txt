# Boost.Test runner. Test headers are included as <contracts/...> from this directory.
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

# Boost 1.86 still ships boost/timer.hpp but requires this define.
# Used only by the test runner for wall-clock reporting.
target_compile_definitions(sjdTest
    PRIVATE
        BOOST_TIMER_ENABLE_DEPRECATED
)

add_test(
    NAME sjdTest
    COMMAND sjdTest --log_level=message --build_info=yes --report_level=short
)
