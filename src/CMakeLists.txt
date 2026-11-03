# sjd — core static library
add_library(sjd STATIC
    contracts/AsianForwardContract.cpp
    contracts/AsianOptionContract.cpp
    contracts/Cashflow.cpp
    contracts/Contracts.cpp
    contracts/ContractStressor.cpp
    contracts/EuropeanForwardContract.cpp
    contracts/EuropeanOptionContract.cpp
    contracts/Portfolio/CVA/cva.cpp
    contracts/Portfolio/Insurance/PortfolioInsurance.cpp
    contracts/Portfolio/Margin/Margin.cpp
    contracts/Portfolio/Portfolio.cpp
    contracts/Portfolio/RegulatoryCapital.cpp
    contracts/Portfolio/PortfolioEvolver.cpp
    contracts/Portfolio/RegCap/RegCapCreditParameters.cpp
    contracts/Portfolio/RegCap/RegCapUnderlyingFactors.cpp
    contracts/Portfolio/Structures/Structure.cpp
    maths/maths.cpp
    maths/TwoDimensionalInterpolation.cpp
    pricing/AsianOptionFormula.cpp
    pricing/Black76Formula.cpp
    rates/Discount/DeterministicDiscountRateSource.cpp
    rates/Discount/DiscountCurves.cpp
    rates/Discount/DiscountRateSource.cpp
    rates/Discount/InterestRateConvention.cpp
    rates/Fixing/FixingCurves.cpp
    rates/Fixing/FixingRateSource.cpp
    rates/Fixing/RollingFixingRateSource.cpp
    rates/Forward/Deterministic/ForwardCurveCMADeterministic.cpp
    rates/Forward/Deterministic/ForwardCurveDeterministic.cpp
    rates/Forward/Deterministic/ForwardCurveDeterministicFX.cpp
    rates/Forward/ForwardCurve.cpp
    rates/Forward/ForwardCurveFactory.cpp
    rates/Forward/ForwardCurveFX.cpp
    rates/Forward/ForwardRateSource.cpp
    rates/Forward/ForwardRateSourceDeterministic.cpp
    rates/Forward/ForwardRateSourceStochastic.cpp
    rates/Forward/Stochastic/ForwardCurveCMAStochasticFwdVol.cpp
    rates/Forward/Stochastic/ForwardCurveStochasticFX.cpp
    rates/Forward/Stochastic/ForwardCurveStochasticFwdVol.cpp
    rates/Forward/Stochastic/ForwardCurveStochasticSpotVol.cpp
    rates/PathGenerators/Path.cpp
    rates/PathGenerators/SingleFactorGMB.cpp
    rates/RatesEnvironment.cpp
    rates/RateSource.cpp
    rates/Volatility/VolatilityRateSource.cpp
    rates/Volatility/VolatilitySurfaceMoneynessCMA.cpp
    rates/Volatility/VolatilitySurfaceDelta.cpp
    rates/Volatility/DeterministicVolatilityRateSource.cpp
    rates/Volatility/VolatilitySurfaceMoneyness.cpp
    rates/Volatility/VolatilityCurve.cpp
    rates/Volatility/VolatilityCurveCMA.cpp
    rates/Volatility/VolatilitySurfaces.cpp
    tools/CalendarMonthAverageCurve.cpp
    tools/Calendars.cpp
    tools/Common.cpp
    tools/Curve.cpp
    tools/DatedCurve.cpp
    tools/Errors.cpp
    tools/Validation.cpp
)
add_library(sjd::sjd ALIAS sjd)

# The static library is linked into the Python extension, a shared module.
set_target_properties(sjd PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Public headers live in include/sjd so callers use <sjd/contracts/...>
# regardless of the repository folder name.
target_include_directories(sjd
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(sjd
    PUBLIC
        QuantLib::QuantLib
    PRIVATE
        "$<BUILD_INTERFACE:sjd_warnings>"
)

target_compile_definitions(sjd
    PUBLIC
        QL_NO_AUTOLINK
        BOOST_ALL_NO_LIB
)
