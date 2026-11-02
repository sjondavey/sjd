#pragma once

#include <boost/test/unit_test.hpp>
#include <boost/timer.hpp>

#include <iomanip>
#include <iostream>

#include <maths/MathsTest.h>
#include <maths/TwoDimensionalInterpolationTest.h>

#include <pricing/AsianOptionFormulaTest.h>
#include <pricing/Black76FormulaTest.h>

#include <rates/Discount/DiscountCurvesTest.h>
#include <rates/Discount/InterestRateConventionTest.h>
#include <rates/Fixing/FixingRatesTest.h>
#include <rates/Forward/CalendarMonthAverageTest.h>
#include <rates/Forward/ForwardCurveFXTest.h>
#include <rates/Forward/ForwardCurveTest.h>
#include <rates/Volatility/VolatilityCurveTest.h>
#include <rates/Volatility/VolatilitySurfacesTest.h>

#include <contracts/AsianOptionTest.h>
#include <contracts/CashflowTest.h>
#include <contracts/ForwardContractTest.h>
#include <contracts/OptionContractTest.h>
#include <contracts/Portfolio/PortfolioEvolverTest.h>
#include <contracts/Portfolio/PortfolioTest.h>
#include <contracts/Portfolio/Structures/StructureTest.h>

#include <contracts/Portfolio/CVA/CVATest.h>
#include <contracts/Portfolio/Insurance/PortfolioInsuranceTest.h>
#include <contracts/Portfolio/Margin/MarginTest.h>

#include <golden/GoldenValuesTest.h>

// QuantLib is linked by CMake. ql/auto_link.hpp picks the wrong .lib name
// for the static-md triplet (it treats /MD as "QuantLib is a DLL").