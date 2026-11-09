# Rates layer

Step 16. The environment, the calendars, and the stresses say what they hold.
The interpolators stay.

## Environment

`RatesEnvironment` kept a boolean beside each curve pointer. `setFRS` set the
boolean even when the pointer was empty, and `hasFRS` trusted the boolean.

`hasFRS`, `hasDRS`, `hasForeignDRS`, `hasVRS` and `hasFixingRS` are the
pointers. An empty pointer is not set. Passing an empty pointer clears that
slot. The setters take `const std::shared_ptr<T>&`.

There is no builder. Callers already assemble an environment with the setters
and check it with `validate()`.

## Forwards

A forward is already one `ForwardCurve`: averaging, roll, and an optional
interest-rate differential, chosen by `makeForwardCurve`. The older class
names are the objects a roll returns, and the tests construct them. This step
does not move or delete those headers, and it does not add another cross
product.

`LinearArrayInterpolator`, `CubicSplineInterpolator` and
`TwoDimensionalInterpolation` stay. QuantLib's interpolators are not a
replacement here. The cubic end conditions, and the golden numbers that depend
on them, stay as they are.

## Calendars and day counters

`QuantLib::Calendar` and `QuantLib::DayCounter` are handles. The library
stores and accepts them by value. A missing one is an empty handle
(`empty()`), not a null `shared_ptr`.

That covers the volatility sources, `InterestRateConvention`,
`CalendarMonthAverageCurve`, and the evolver's holding-period calendar.

## Stresses

Every `RateSource` has `marketStresses()`. The default is an empty list.
Deterministic discount, forward, and volatility sources return the stresses
they already built. `ContractStressor` calls that method. It no longer casts
to `IHaveMarketRateStresses`, and that type goes away.

`parallelBump` keeps its meaning. A forward bump adds to the forward, a
volatility bump adds to the volatility, and a discount bump takes an
`InterestRateConvention` by const reference because it bumps the interest rate
and converts back to a discount factor. One shared signature would change that
conversion.
Stochastic sources return no stresses, which is what the cast did before.
