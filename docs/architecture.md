# Architecture

Four layers. C++ prices and simulates. Python turns that into books,
reports, and a parameter search.

## Contracts

`Contract` can be validated against a `RatesEnvironment` and then valued.
`validate` records issues. `isOK` only reads that report. `getValue`
assumes validation has already succeeded and does not store the result.

A single instrument inherits `SingleInstrumentContract`: a settlement, a
volume, a buy or sell, and a payoff. The priced instruments are
`EuropeanOption`, `EuropeanForward`, `AsianOption`, `AsianForward`, and
`FixedCashflow`. `Cashflow` is the same class as `FixedCashflow`.

A group inherits `MultipleInstrumentContract`. `Portfolio` is that group.
It holds the instruments and, optionally, one `Margin`. It has no capital
method. `CurrentExposureMethod` is built beside the portfolio and reads
the margin balance it is given.

A user-facing parametrised book is a strategy (`CollarStrategy`,
`StripStrategy`, `SeagullStrategy`, `ParticipatingForwardStrategy`). A CEM
netting set is `CemNettingSet`. Those are different objects.

## Rates

`RatesEnvironment` has one anchor date and five slots: forward, domestic
discount, foreign discount, volatility, and fixings. An empty pointer
means the slot is unset.

`ForwardCurve` is the forward. Averaging, the roll, and an optional
interest-rate differential are chosen when it is built. A path that rolls
with forward volatility needs that roll. A deterministic roll cannot drive
a simulation.

`DiscountCurve` and `VolatilityCurve` are pillar curves.
`VolatilitySurfaceMoneyness` is the strike-dependent surface. Moneyness
there is `(strike - forward) / forward`. The array and surface
interpolators are the library's own. They are not QuantLib interpolators.

Calendars and day counters are QuantLib handles, stored by value. A
missing calendar is an empty handle.

## Simulation

`simulate` builds a `PortfolioEvolver` on a `GbmPathGenerator`. The default
draws are a seeded `mt19937` and Box-Muller. `sampling="sobol"` uses a
Brownian bridge. Path `i` starts at a fixed offset in the sequence, so one
thread and many threads write the same shocks.

Each path stores the spot, the mark-to-market, and the margin balance when
the book is margined. The result is the paths, not only their means.
Exposure and CVA in Python are means of those paths.

`by_netting_set=True` needs an object with both a portfolio and a
`capital_model`. The evolver then stores each netting set's value.
`by_structure` is the same array. A plain `Portfolio` has no netting sets.

`value_on_spots` marks a book to a spot array the caller already drew, so
a search can compare strategies on one set of paths.

## Where Python takes over

`python/sjd_module.cpp` binds the C++ types and re-exports them from
`sjd`. Dates accept a `date`, a `numpy.datetime64`, or a pandas
`Timestamp`. `sjd.SjdError` is the translated `sjd::Error`.

Python then owns the rest of the front end:

| Module | Role |
| --- | --- |
| `sjd.stats` | Exposure, CVA, DVA, funding, and CEM along a simulation |
| `sjd.strategy` | Collar, strip, seagull, participating forward |
| `sjd.optimise` | Common-random-number search |
| `sjd.book` | TOML book |
| `sjd.reports` | Prices, exposure, costs, capital |
| `sjd.markets` | Catalogue, pillars, and a rates environment |
| `sjd.examples` | The sample market used by the notebooks |
| `python -m sjd run` | Price a book and write a report folder |

`pandas`, `openpyxl`, and `scipy` are the `front` extra. The core install
does not import them.
