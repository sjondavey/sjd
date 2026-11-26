# Python bindings

Step 19. The classes below are bound, tested, and named in the module
docstrings. Names follow the C++ type. A method that only reads state does
not change that state.

## Cashflow

`FixedCashflow` is the single cashflow contract. Maturity and settlement are
the same date. The amount is the volume, signed by buy/sell, discounted on
the environment's discount curve. `Cashflow` is the same class.

## FX

`RatesEnvironment.set_foreign_drs` and `foreign_drs` hold the foreign
discount curve. `set_drs` stays the domestic curve. `frs`, `drs`, `vrs`,
and `fixing_rs` read the curves back. An empty slot is `None`.

`EuropeanOption`, `AsianOption`, `EuropeanForward`, and `AsianForward`
expose `strike`. The two options also expose `type`, `premium_amount`, and
`premium_payment_date()`.

`ForwardCurveFX` is spot plus the two discount curves. Pass a spot date and
the spot is moved to the anchor by the two discount factors, or pass the spot
already on the anchor. `ForwardCurveDeterministicFX` is that curve as a
deterministic forward. `ForwardCurveStochasticFX` adds a volatility source so
a portfolio evolver can drive it. All three are forward curves, so
`set_frs` accepts them.

## Stresses

`ContractStressor.stressed_values()` returns a dict of stress name to value.
The C++ method returns pairs. A dict is the same data and can be turned into
a pandas Series without pandas being a dependency.

## Insurance

`SimpleInsurance` is a constant sum insured between the policy dates.
`SimpleInsuranceWithTermStructure` is a dated schedule of sums insured.
`sum_insured(date)` and `insurance_payments(dates, mtm)` are the two
calculations. The base class is not constructed.

## Volatility and discount

`VolatilitySurfaceMoneyness` matches `VolatilitySurfaceMoneynessCMA`: rows
are moneyness, columns are observation dates, and a moneyness outside the
grid reads the nearest wing.

`InterestRateConvention` converts a discount factor and a rate. The day
counter is Actual/365, which is the day counter the other Python curves use.
`Compounding` and `Frequency` are the QuantLib enumerations.
`DiscountCurve.parallel_bump(spread, convention)` returns a new discount
curve. No other discount class is unbound: `DiscountCurve` is already the
curve Python builds.

## Asian formula

`AverageRateCall` and `AverageRatePut` take the live forwards, the fixed
rates, the strike, the standard deviations of the live forwards, and the
discount factor to settlement. `premium()` is the price. One live forward
and no fixings is the Black-76 price of that forward.

## Margin parameters

`Margin.set_margin_parameters` is the full argument list: both thresholds,
both initial margins, the minimum transfer amount, and the rounding. It
resets the balance to zero. `SimpleMargin.get_margin_call` still uses
variation margin, the thresholds, the minimum transfer, and the rounding.
The initial-margin amounts are stored and checked. A negative one fails
`validate`. They do not change the call.

## Capital

`CapitalResult` is already bound with exposure, risk-weighted assets,
capital, replacement, the net-to-gross ratio, and one dict per netting set.
This step adds its text form and a test that `capital()` returns that
breakdown. The fields do not change.

## Pickle

`py::pickle` rebuilds an object from the inputs it was constructed with, so
a book can be sent to another process.

Contracts, `SimpleMargin`, and `Portfolio` pickle from their public fields.
`set_margin_parameters` resets the balance, so the balance is stored beside
the parameters and put back after construction.

A discount curve pickles its dates, factors, interpolator, and extrapolation.
A volatility curve pickles the volatilities it was given, not the variance
it interpolates. A fixing curve pickles its dates and rates. A surface
pickles its dates, strike grid, volatility grid, interpolator, and
extrapolation.

Every forward subclass holds a `ForwardCurve`. `quoteState()` returns it.
Pickle reads the quote style, the pillars or the spot and the two discount
curves, the roll, and the volatility. An interest-differential curve is
restored from the spot already on the anchor, not from the original spot
date. The forward is the same.

`RatesEnvironment` pickles its anchor and whichever of the five sources is
set. A source that cannot be pickled fails the environment with it.

## Text

Every bound class has `__repr__`. It names the type and the fields that
identify the object. A curve names its anchor and how many pillars it has,
not the whole grid.
