# Notebook plan

A plan for the tutorial notebooks in `python/notebooks`. It says what each
notebook shows, in the order a reader meets the library: market rates, then
trades and portfolios, then simulation, then statistics. Validation, logging,
and storing and reloading objects each get a notebook of their own and an
explicit example in the notebooks around it.

Notebooks 01 to 11 are written. The eleven notebooks that were in
`python/notebooks` have been removed. Notebooks 12 onward are not written yet.
The plan comes before Phase 6 of `MODERNISATION_TODO.md`, so the SA-CCR
notebook in Step 22 has a settled series to join.

## The notebooks that were replaced

Read against the code after Step 21, which is why they were removed:

- Every one of the eleven uses a name that Step 10 or Step 14 deprecated
  (`is_ok()`, `error_messages()`, `validate_report()`, `validation_report()`,
  `PortfolioB2CEM`, `by_structure`), or the build instructions that Step 18
  replaced ("copies `_core.pyd` into `python/sjd`").
- Committed outputs are stale. Several print `dir(sjd)` and list
  `RegulatoryCapital` and `RegCapUnderlyingFactors`, which are no longer bound.
- `sample_database.ipynb` deletes and rewrites `python/sjd/markets/sample.db`,
  a tracked file inside the package.
- Nothing shows logging, `simulation.seed`, `simulation.metadata`,
  `build_info`, pickle, the TOML book, `markets.rates_environment`, a
  volatility surface, `ForwardCurveFX`, a strategy, the optimiser, `sjd.reports`,
  the command line, `ContractStressor`, insurance, or
  `CurrentExposureMethod` used directly.

## Rules for every notebook

1. **Named `NN_topic.ipynb`**, so the folder sorts in reading order.
2. **Runs in CI.** The linux-release job runs
   `pytest --nbmake python/notebooks --nbmake-timeout=180` with
   `MPLBACKEND=Agg`. Every Monte Carlo is seeded and small. A notebook only
   imports what that job installs: numpy, pandas, scipy, openpyxl, matplotlib.
3. **Asserts what it claims.** A statement such as "the portfolio value is the
   sum of the legs" is followed by an `assert`. The notebook is also a test.
4. **Writes only to a temporary directory.** Nothing writes into the
   repository or the installed package.
5. **Current names only**: `validate`, `is_valid`, `report`, `require_valid`,
   `Portfolio` with `CurrentExposureMethod`, `by_netting_set`. The deprecated
   names appear once, in a table in notebook 09.
6. **Starts the same way**: `logging.basicConfig(level=logging.INFO)` and
   `sjd.set_log_level("INFO")`, so library records appear under the cell that
   caused them.
7. **Shows a report as a report.** `ValidationReport` has `_repr_html_`, so a
   cell that ends with a report renders a table. No `show_report` helper.
8. **Ends with up to three fixed headings** where they apply: *What can go
   wrong* (validation), *What it logs*, and *Save and reload*.

## Running example

Two markets, both anchored on 16 September 2026.

The **sample market** is `sjd.examples.sample_market()`: one FX-style curve,
spot 17.50, domestic rate 7%, foreign rate 4%, volatility 12%. The **sample
book** is a 97/103 collar, an outright forward struck at 98% of the curve, and
an Asian forward struck at 99%. It is written twice, as
`examples.sample_contracts` and as `python/sjd/books/sample.toml`, and
notebook 08 asserts the two give the same values. Notebooks 02 to 04 build this
market by hand. From 06 onward it comes from `examples`.

The **catalogue markets** are XAU, XPT, DATED_BRENT, and USDZAR, from
`sjd/markets/catalogue.toml` with the dummy pillars in `sjd.markets.sample`.
Notebook 05 uses them to show curves built from stored pillars.

## The series

| No. | Notebook | Shows |
| --- | --- | --- |
| | **Market rates** | |
| 01 | `01_getting_started` | Install, version, dates, calendars, switching logging on |
| 02 | `02_curves` | Discount, forward, volatility, and fixing curves |
| 03 | `03_volatility_surfaces` | Delta and moneyness surfaces |
| 04 | `04_rates_environment` | The five slots, deterministic and stochastic forwards, FX |
| 05 | `05_market_catalogue_and_store` | Markets in TOML, pillars in SQLite, an environment from both |
| | **Trades and portfolios** | |
| 06 | `06_contracts` | Each instrument, checked against its formula, and stresses |
| 07 | `07_portfolios_and_strategies` | Portfolios, margin, and the four strategies, in Python |
| 08 | `08_book_files` | The same portfolios as TOML markup |
| 09 | `09_validation_and_errors` | Reports, issue codes, and the exception types, in one place |
| | **Simulation** | |
| 10 | `10_simulation` | `simulate`, the result arrays, spot paths, and the options |
| 11 | `11_logging_and_provenance` | Every log record, and where a number came from |
| | **Statistics** | |
| 12 | `12_exposure_statistics` | EE, PFE, quantiles, VaR, standard errors |
| 13 | `13_valuation_adjustments` | Credit curves, CVA, DVA, funding, insurance, structure costs |
| 14 | `14_regulatory_capital` | Basel II current exposure, today and along the paths |
| | **Front end and storage** | |
| 15 | `15_strategy_search` | Common random numbers and the two searches |
| 16 | `16_reports_and_command_line` | Report frames, CSV, Excel, `python -m sjd run` |
| 17 | `17_save_and_reload` | Store a whole run, reload it, and reproduce it |

## Market rates

### 01 `01_getting_started`

What must be true before any other notebook runs.

- Install with the `front` extra. `sjd.__version__` and `sjd.build_info()`:
  compiler, build type, QuantLib and Boost versions, git commit.
- A date can be a `datetime.date`, a `numpy.datetime64`, or a
  `pandas.Timestamp`. One lookup with each gives the same answer.
- `sjd.Calendar`: `Calendar.names()`, one named calendar, the joint USDZAR
  calendar, `is_business_day`, `adjust`, `advance`, `business_days_between`,
  `holidays`.
- The sample dates derived the way `examples.sample_market` derives them:
  collar settlement three months out, maturity two business days earlier.

**Validation.** `sjd.Calendar("Narnia")` raises `sjd.ConfigurationError`. It
is also a `ValueError` and carries `.code == "CONFIGURATION"`. This is the
first sight of the exception family.

**Logging.** Switched on here. Nothing is logged yet.

### 02 `02_curves`

The pillar curves, built by hand from the flat rates of the sample market.

- `DiscountCurve`, `ForwardCurve`, `VolatilityCurve`, then the same three from
  `examples.fx_curves`. `get_discount_factor`, `get_forward`,
  `get_volatility`, plotted between the pillars.
- Linear against `CurveInterpolator.CUBIC` on the forward. A flat volatility
  stays flat before the first pillar, because the curve interpolates total
  variance.
- `allow_extrapolation` off and on.
- Calendar-month curves: `averaging=ForwardAveraging.CALENDAR_MONTH` and
  `VolatilityCurveCMA`, where every day of a month reads the same value.
- `DailyFixingCurve` and `get_rate`.
- `InterestRateConvention` (`rate_to_df`, `df_to_rate`) and
  `DiscountCurve.parallel_bump`.

**Validation.** A curve with dates and values of different lengths is built,
then `validate()` returns the interpolator's dimension code under the curve's
path. `is_valid` and `report` read that result and do not re-run it. A curve
that extrapolates is valid and carries a `WARNING`: `report.ok` stays true.
A calendar-month forward with `ForwardRoll.SPOT_VOLATILITY` raises
`ConfigurationError`.

**Logging.** The extrapolation warning and the flat back-fill to the anchor
also arrive as `WARNING` records on `sjd._core`.

**Storage.** Each curve through `pickle.dumps` and `pickle.loads`, with the
restored curve reading the same values.

Replaces the curve cells of `european_option`, `currency_forward_strip`, and
`asian_forward_cma`.

### 03 `03_volatility_surfaces`

Strike-dependent volatility.

- `VolatilitySurfaceDelta`: rows are put deltas in percent, 50 is at the
  money. `get_volatility(date, strike, forward)` and `strike_from_delta`.
- `VolatilitySurfaceMoneyness` and `VolatilitySurfaceMoneynessCMA`: rows are
  `(strike - forward) / forward`. A strike outside the grid reads the nearest
  wing. `extrapolate` extends time only.
- `SurfaceInterpolator.BILINEAR` against `BICUBIC`, as a smile plot.
- A surface passed where a curve is expected: as an environment's volatility
  and as a forward's `volatility=`.

**Validation.** A grid whose rows do not match the delta list reports
`VOLATILITY_SURFACE_DIMENSION_MISMATCH`. Strikes out of order report
`VOLATILITY_SURFACE_STRIKES_NOT_INCREASING`. A delta that cannot be solved
raises `sjd.NumericalError`, which
is an `ArithmeticError` and not a `ValueError`.

**Logging.** The failed solve writes `Could not convert a delta to a strike`
at `WARNING`. The notebook captures the record and shows it.

**Storage.** A surface through pickle.

### 04 `04_rates_environment`

One anchor date and five slots.

- `RatesEnvironment(anchor)`, `set_frs`, `set_drs`, `set_foreign_drs`,
  `set_vrs`, `set_fixing_rs`.
- The same pillars as a deterministic forward and with
  `roll=ForwardRoll.FORWARD_VOLATILITY`. Both price identically. Only the
  second can drive a path, which notebook 10 shows.
- FX: `ForwardCurveFX` with a spot date moved to the anchor,
  `ForwardCurveDeterministicFX`, `ForwardCurveStochasticFX`, and
  `foreign_drs()`.
- `examples.sample_market()` and `sample_market(stochastic=True)`: the dict
  and its keys. Later notebooks start from this call.

**Validation.** An empty environment reports `RATES_ENVIRONMENT_NO_RATES`. A
curve on another anchor date reports the anchor-mismatch code for its slot,
such as `DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH`. A
broken discount curve appears under the environment's path, so the reader sees
a child report absorbed by its owner. `sjd.require_valid(environment)` turns
the same report into a `ValidationError`.

**Storage.** The environment pickled to a file and reloaded. A forward priced
on both gives the same value.

Replaces the environment cells of `european_option` and the nested-failure
section of `validation_reporting`.

### 05 `05_market_catalogue_and_store`

Market conventions as markup, pillars in a database, and an environment built
from the two.

- The catalogue: `markets.CATALOGUE`, `market_spec`, `quote_spec`, read beside
  `catalogue.toml`. Forward kinds (`gofo`, `outright`, `metal_times_usdzar`),
  volatility kinds (`delta`, `calendar_month_moneyness`), date kinds.
- A new market in markup only: `parse_catalogue(text)` with one more entry,
  and no Python change.
- The store: a new SQLite file in a temporary directory,
  `markets.load_sample(connection)`, and the tables in `schema.sql`.
- Retrieval: `store.spot`, `forward_pillars`, `discount_pillars`,
  `volatility_grid`.
- `markets.rates_environment(market, anchor, currency, connection, roll)` for
  XAU USD (lease rates turned into outrights), XPT ZAR (a cross), DATED_BRENT
  USD (calendar month), and USDZAR. One contract priced on each.
- Date conventions: `spot_date`, `option_expiry`, `pricing_dates`,
  `month_settlement`.

**Validation.** `PillarNotFound` for an anchor with no rows.
`QuoteNotAvailable` for DATED_BRENT in ZAR. `CatalogueError` for a malformed
catalogue. `SchemaVersionError` for a database from an older schema. A pillar
row for a quote the catalogue does not store is refused by the foreign key. A
calendar-month market asked for a spot-volatility roll is a `ValueError`.

**Logging.** Each load writes `loaded market ...` at `INFO` on
`sjd.markets.rates`.

**Storage.** This is the storage notebook for market data: TOML for
conventions, SQLite for numbers, written and read back.

Replaces `sample_database`.

## Trades and portfolios

### 06 `06_contracts`

Each instrument, priced and checked against its formula.

- `EuropeanForward`, both constructors: physically settled, and cash settled
  with a separate rate-set date.
- `EuropeanOption`: `get_option_value` against `Black76Call` and `Black76Put`,
  and `get_value` with the premium and the buy or sell sign.
- `AsianForward` and `AsianOption` on calendar-month curves, with fixings
  before the anchor. `AverageRateCall` and `AverageRatePut`, and the case that
  reduces to Black-76.
- `FixedCashflow`, and `Cashflow` as the same class.
- `get_payoff`, `maturity_date`, `settlement_date`, `volume`, `buy_sell`, and
  `repr`.
- `ContractStressor(contract, environment).stressed_values()` as a bar chart.

**Validation.** A negative strike with an empty environment gives several
issues in one report, the contract's own input first. `validate(environment)`
against a market with no volatility reports the missing source. `is_valid`
only reads the last report, so the notebook shows it lagging until `validate`
is called again. `Black76Call(0.0, ...)` raises `ConfigurationError`: a
formula is not a `Validatable`.

**Storage.** One contract of each type through pickle.

Replaces `european_option`, `currency_forward_strip`, `asian_forward_cma`, and
the Black-76 check in `european_option_collars`.

### 07 `07_portfolios_and_strategies`

A book built in Python.

- `Portfolio(contracts)`: `contracts()`, `get_value`, and the value as the sum
  of the legs.
- The sample book from `examples.sample_contracts`.
- Strategies: `CollarStrategy`, `StripStrategy`, `SeagullStrategy`,
  `ParticipatingForwardStrategy`. For each: `from_moneyness`, `contracts()`,
  `portfolio()`, `premium(environment)`, and a payoff diagram from
  `get_payoff`. `merge_contracts` to combine them.
- Margin: `SimpleMargin` thresholds, minimum transfer amount, and rounding, on
  three mark-to-markets. `Portfolio(contracts, margin)`, `is_margined`,
  `get_margin_call`, `set_margin_balance`. `set_margin_parameters` stores
  initial margin and does not change the call.

**Validation.** A portfolio against an empty environment gives one report
whose paths name the contract index (`portfolio`, `contracts[0]`,
`european_option`). A strategy with the put above the call raises
`ValueError` when it is constructed. A negative initial margin fails
`margin.validate()`.

**Logging.** Each strategy writes `built ... portfolio` at `INFO` on
`sjd.strategy`.

**Storage.** A margined portfolio through pickle, with the margin balance
restored. A strategy is a frozen dataclass of dates and numbers. The notebook
checks that it pickles and writes its fields out as a plain dict.

Replaces `portfolio_valuation`, the margin rules in `margining`, and the collar
strip in `european_option_collars`.

### 08 `08_book_files`

The same portfolios as TOML markup, read by `sjd.book`.

- `python/sjd/books/sample.toml` read line by line: `[book]`, `[[trades]]`,
  `kind`, `schedule`, moneyness.
- `load_book(path, market)` and `parse_book(text, market)`. `Book.name`,
  `trades`, `contracts()`, `portfolio()`.
- Assert that the sample file and `examples.sample_contracts` price to the
  same values, trade by trade.
- Every `kind` once, in one book written in the notebook: `cashflow`,
  `european_forward`, `european_option`, `asian_forward`, `asian_option`,
  `collar`, `strip`, `seagull`, `participating_forward`.
- Absolute strikes against moneyness. `schedule` against explicit dates. The
  `[book]` default `volume`. Omitted premium date and amount.
- `[margin]`: the table becomes a `SimpleMargin` on the market anchor, and
  `book.portfolio()` is margined.

**Validation.** `BookError` names the trade and the field. The notebook
triggers each family: text that is not TOML, an unknown `kind`, an unknown
field, a missing volume, both `strike` and `strike_moneyness`, a moneyness
with no market, and a collar whose put is above its call. A book that loads is
still validated against the market with `require_valid(book.portfolio(),
environment)`: loading checks the file, validation checks the trades.

**Storage.** This is the storage notebook for trades. A book is written to a
temporary file, loaded, pickled, and loaded again, with the same values each
time. See gap 3 below: the file is read, never written.

### 09 `09_validation_and_errors`

The error-handling policy from `docs/error-handling.md`, as running code. By
this point the reader has met each piece once. This notebook puts them in one
place.

- A report in full: `ok`, `issues`, and for each issue `code`, `severity`,
  `path`, `message`. `str(report)` is one `path: code: message` line per
  issue. `repr`, the HTML table, `to_dict()`.
- `ERROR` against `WARNING`. A warning does not fail `ok`.
- `validate()` runs the checks. `is_valid` and `report` read the last result.
- An object that has never been validated has an empty report, and an empty
  report is ok. A new, empty `RatesEnvironment` reads `is_valid == True`
  until `validate()` reports `RATES_ENVIRONMENT_NO_RATES`. See gap 9.
- How a path nests: interpolator, curve, environment, contract, portfolio.
  One broken curve at the bottom, read from each level above.
- `report.raise_if_invalid()` and `sjd.require_valid(obj, env)`. The
  `ValidationError` carries `.report`.
- The exception family as a table and as `except` clauses: `SjdError` with
  `.code`, `ConfigurationError` and `ValidationError` as `ValueError`,
  `NumericalError` as `ArithmeticError`, `BookError`, and the catalogue and
  store errors from notebook 05.
- The gate before a simulation: a `PortfolioEvolver` on a deterministic
  forward reports `PATH_GENERATOR_FORWARD_NOT_STOCHASTIC`, and
  `sjd.simulate` on the same inputs raises `ValidationError` with that report.
- The contract: validation runs once at setup. `get_value` assumes it passed.
- Where to look a code up: `docs/issue-codes.md`.
- The deprecated names and what replaces each, as a table. Each is called
  once to show it still works.

**Storage.** `json.dumps(report.to_dict())` to a file and back, as the way to
keep a failed report beside a rejected run.

Replaces `validation_reporting` and `portfolio_validation`.

## Simulation

### 10 `10_simulation`

From a valid book and a stochastic market to arrays of paths.

- What `simulate` needs: the first date is the anchor, and the forward rolls.
- `sjd.simulate(portfolio, environment, dates, paths, seed=...)` and the
  result: `dates`, `spot`, `mtm`, `margin`, `paths`, `steps`, `trades`, shaped
  `[path, step]`. A fan chart of spot and of mark-to-market.
- `by_trade=True`, and the trades summing to `mtm`.
- `antithetic`: path `i` and path `i + paths // 2` mirror each other.
- `seed`: the same seed repeats the arrays. An omitted seed is drawn and
  reported.
- `threads`: one thread and four threads give identical arrays.
- `sampling="sobol"` against `"pseudo"`: the error of the mean at equal paths.
- `aggregates=True`: per-step `ee`, `pfe`, and means, without the path cube.
  `float32=True` halves the memory.
- A margined book: `margin` is an array, and `holding_period` with a calendar
  adds the margin reset dates. `exposure_steps` and `margin_reset_steps`.
- Without random numbers: `PortfolioEvolver.value_spot_path` on the curve's
  own forwards, and `value_spot_paths` on an array the caller supplies.

**Validation.** Three runs that are refused, each raising `ValidationError`
with its report: a deterministic forward
(`PATH_GENERATOR_FORWARD_NOT_STOCHASTIC`), dates that do not start on the
anchor (`SIMULATION_ANCHOR_MISMATCH`), and a last date past the curve
(`PATH_GENERATOR_DATES_AFTER_END`). A holding period with no calendar is a
plain `ValueError`: an argument mistake, not a validation issue.

**Logging.** `simulation start` and `simulation end` at `INFO`, with paths,
dates, seed, and the two timings.

**Storage.** `numpy.savez` of the arrays, as a pointer to notebook 17.

Replaces the path sections of `portfolio_paths`.

### 11 `11_logging_and_provenance`

What the library says while it works, and how to trace a number back.

- The loggers: `sjd._core` for C++ records, and `sjd.markets.rates`,
  `sjd.stats`, `sjd.strategy`, `sjd.optimise` for Python.
- `sjd.set_log_level`: `"INFO"`, `"WARNING"`, `"ERROR"`, `"OFF"`. It sets the
  C++ level and the `sjd` logger together.
- Every record the library writes, each triggered once and shown in a table:

  | Level | Logger | Record | Trigger |
  | --- | --- | --- | --- |
  | INFO | `sjd._core` | `simulation start`, `simulation end` | `simulate` |
  | INFO | `sjd._core` | `CEM optimisation` | `CurrentExposureMethod(..., optimise=True)` |
  | INFO | `sjd.markets.rates` | `loaded market` | `rates_environment` |
  | INFO | `sjd.strategy` | `built ... portfolio` | `strategy.portfolio()` |
  | INFO | `sjd.stats` | `summary dates=` | `stats.summary` |
  | INFO | `sjd.optimise` | `optimiser trial`, `optimiser parameters=` | the two searches |
  | WARNING | `sjd._core` | extrapolation past the last pillar | validating such a curve |
  | WARNING | `sjd._core` | flat back-fill to the anchor | validating such a curve |
  | WARNING | `sjd._core` | delta and strike conversion failures | `strike_from_delta` |

- A `logging.FileHandler` on `sjd`, a run, and the file read back.
- Filtering with standard `logging`: silence `sjd.optimise` and keep
  `sjd._core`.
- Why simulation records arrive together: they are queued while the GIL is
  released and flushed when `simulate` returns.
- `DEBUG` and `TRACE` are compiled out of a Release build, which is what
  `pip install` produces. The notebook says so and does not pretend to show
  them. See gap 6.
- Provenance: `simulation.seed`, and `simulation.metadata` with its version,
  git commit, build type, `timings`, `warnings`, and `inputs`. A run repeated
  from `metadata["inputs"]` gives identical arrays.
- `stats.summary(simulation, metadata=True)` and
  `stats.structure_costs(..., metadata=True)`.

**Validation.** A run on an extrapolating curve succeeds, and the warning is
in `metadata["warnings"]` as well as in the log.

**Storage.** `metadata` written as JSON beside the saved arrays.

## Statistics

### 12 `12_exposure_statistics`

`sjd.stats` on a `[paths, steps]` array.

- `exposure(simulation)`: mark-to-market less margin. `exposure_dates`.
- `expected_exposure`, `negative_expected_exposure`,
  `potential_future_exposure`, `negative_potential_future_exposure`,
  `quantiles`. One chart of the profile with its bands.
- `value_at_risk` and `expected_shortfall` between the anchor and a step.
- `standard_error`, with and without `antithetic`, and why the pairs are the
  sample.
- `summary(simulation)` as a pandas frame.
- One trade at a time: the same functions on `simulation.by_trade[:, :, k]`.
- Margined against unmargined on the same seed, and
  `holding_period_exposure`.
- The path statistics against `aggregates=True` from notebook 10.

**Validation.** The functions check their arguments and raise `ValueError`: a
level outside `(0, 1)`, an array that is not two-dimensional, an odd path
count with `antithetic=True`.

**Logging.** `summary dates=` on `sjd.stats`.

Replaces the Monte Carlo table in `portfolio_paths`.

### 13 `13_valuation_adjustments`

From exposure to cost.

- Credit curves: `FlatDefaultProbability` and
  `CumulativeDefaultProbability`, `get_pd`, `get_lgd`, `extends_to`.
  `stats.flat_default_probability` and `stats.credit_curve_terms`.
- `credit_valuation_adjustment` and `debit_valuation_adjustment`, on a profile
  and on a panel, and why the mean of the pathwise adjustments is the one the
  command line reports.
- `cost_of_funding` against a spread curve built with `parallel_bump`.
- `structure_costs`: `cva`, `dva`, `funding`, `total`, and the profiles.
- Insurance: `SimpleInsurance` and `SimpleInsuranceWithTermStructure`,
  `sum_insured` and `insurance_payments` on a simulated profile.

**Validation.** A cumulative default probability that falls reports
`CVA_PD_NOT_INCREASING`. A loss given default above one reports
`CVA_LGD_RANGE`. Discount factors of the wrong length are a `ValueError`.

### 14 `14_regulatory_capital`

Basel II current exposure, the capital model the library has today. This is
the baseline that the SA-CCR notebook is compared against.

- `sjd.reg_cap_factors("commodity")`, `SupervisoryFactors.get_factor`, and
  `CounterpartyCredit(risk_weight, capital_ratio)`.
- `CurrentExposureMethod(portfolio, factors, credit, optimise)` built beside
  the `Portfolio`. `capital(portfolio, margin, environment)` returns a
  `CapitalResult`: `ead`, `rwa`, `capital`, `replacement`, `net_gross_ratio`,
  `components`.
- `netting_sets()` with `optimise` off and on for the collar: two sets become
  one, and the add-on falls.
- Commodity against FX factors on the same book.
- Margin: a zero balance, then a balance equal to the mark-to-market.
- Along the paths: `simulate(..., by_netting_set=True)`,
  `stats.regulatory_capital`, and `structure_costs` with `capital_cost_rate`.
  `aggregates=True` and `mean_capital`.
- The three books Step 22 compares (outright, collar, seagull) with their CEM
  capital, so notebook 18 has numbers to set SA-CCR against.

**Validation.** `model.validate(environment)`. `netting_sets()` raises
`ValidationError` when the sets cannot be built. `by_netting_set=True` on a
plain `Portfolio` is a `ValueError`. An unknown asset class in
`reg_cap_factors` raises `ConfigurationError`.

**Logging.** `CEM optimisation netting_sets=...` at `INFO`.

Replaces `regulatory_capital`, the capital half of `margining`, and the
capital columns of `portfolio_paths`. Depends on gap 1.

## Front end and storage

### 15 `15_strategy_search`

Choosing strategy parameters on one set of paths.

- Common random numbers: `optimise.draw_spot_paths` once, then
  `value_on_spots` for two candidate books. The difference between them is
  less noisy than two independent runs, shown as a standard error.
- `evaluate_strategy_cost`.
- `minimise_collar_put_moneyness`: the grid, the `trials`, and the score
  against put moneyness as a chart. `objective` and `premium_weight`.
- `minimise_strategy`: a `build(parameters)` function for a seagull, bounds,
  and the scipy result.

**Validation.** `value_on_spots` calls `require_valid` on the evolver, so a
deterministic market raises `ValidationError` before any trial. An unknown
`objective` is a `ValueError`.

**Logging.** One `INFO` record per trial on `sjd.optimise`. The notebook
turns them into the trial table.

Depends on gap 1 for the capital objective.

### 16 `16_reports_and_command_line`

A priced book as tables and files.

- `reports.prices`, `exposure_profile`, `costs_table`, `capital_table`.
- `reports.write_csv` and `reports.write_excel` into a temporary folder, then
  each file read back with pandas and compared with the frame.
- `python -m sjd run book.toml --market sample --paths ... --output ...`, run
  from the notebook through `sjd.__main__.main([...])`. The folder it writes
  matches the frames built by hand above, for the same seed.
- The flags: `--seed`, `--default-probability`, `--loss-given-default`,
  `--no-capital`.

**Validation.** A book with an error makes the command print the `BookError`
and return 2. A book that loads and fails validation raises
`ValidationError`.

**Storage.** This is the storage notebook for results that people read: CSV
and Excel.

### 17 `17_save_and_reload`

One run, stored completely, then reproduced from what was stored.

- What pickles and what does not, as a table checked by the notebook. From
  the bindings: contracts, `SimpleMargin`, `Portfolio`, every curve, forward,
  and surface, and `RatesEnvironment` do. `Simulation`, `ValidationReport`,
  `CurrentExposureMethod`, `Calendar`, the credit curves, and the insurance
  types have no pickle support.
- Save a run folder: the market as a pickled environment, the book as its
  TOML file and as a pickled portfolio, the arrays with `numpy.savez`,
  `metadata` and the validation report as JSON, the report tables as CSV.
- Start again from only that folder: reload the environment and the
  portfolio, `require_valid`, re-run with the stored seed, and assert the
  arrays equal the stored arrays.
- Compare `sjd.build_info()` with the stored metadata, and say what a
  different git commit would mean for the comparison.
- A book sent to a worker: `multiprocessing` with a pickled portfolio and
  environment, each worker running its own seed.
- Which format for which purpose: TOML for trades and conventions, SQLite for
  pillars, pickle for exact objects between processes, numpy and JSON for
  results, CSV and Excel for people.

**Validation.** A reloaded object is rebuilt from its inputs and has not been
validated. Its report is empty, so the notebook runs `require_valid` on
everything it reloads before it prices anything.

**Logging.** The run's log file is part of the saved folder.

## Coverage of the three threads

Each line is a feature and the notebook that shows it. A feature in bold has
its own notebook.

**Validation and errors (09)**

| Feature | Notebook |
| --- | --- |
| `validate()` returns a report; `is_valid` and `report` only read | 02, 06, 09 |
| Report fields, text, HTML table, `to_dict` | 09 |
| `WARNING` does not fail `ok` | 02, 09, 11 |
| A child report absorbed by its owner; nested paths | 04, 07, 09 |
| A contract's terms against a contract's market | 06 |
| `require_valid`, `raise_if_invalid`, `ValidationError.report` | 04, 08, 09 |
| `ConfigurationError` | 01, 02, 06, 14 |
| `NumericalError` | 03 |
| `BookError` | 08, 16 |
| Catalogue and store errors | 05 |
| The gate before a simulation | 09, 10, 15 |
| Argument mistakes as plain `ValueError` | 07, 10, 12, 13 |
| Credit curve and margin checks | 07, 13 |
| Capital model checks | 14 |
| Issue-code table and deprecated names | 09 |

**Logging (11)**

| Feature | Notebook |
| --- | --- |
| `logging.basicConfig` and `sjd.set_log_level` | 01, 11 |
| Logger names and filtering | 11 |
| Simulation start and end | 10, 11 |
| CEM optimisation | 11, 14 |
| Market load | 05, 11 |
| Strategy built, optimiser trial | 07, 11, 15 |
| Curve warnings | 02, 11 |
| Failed delta solve | 03, 11 |
| Log to a file and read it back | 11, 17 |
| Seed, metadata, `build_info` | 01, 10, 11, 17 |

**Storage and retrieval (17)**

| Feature | Notebook |
| --- | --- |
| Pickle a curve, a surface, an environment | 02, 03, 04 |
| Pickle a contract, a margined portfolio | 06, 07 |
| TOML for market conventions | 05 |
| SQLite for pillars | 05 |
| TOML for trades | 08 |
| A validation report as JSON | 09, 17 |
| Simulation arrays with numpy | 10, 17 |
| Run metadata as JSON | 11, 17 |
| CSV and Excel report tables | 16, 17 |
| A whole run saved, reloaded, and reproduced | 17 |
| Objects sent to another process | 17 |

## What happens to the current notebooks

Each is deleted when the notebook that replaces it lands, in the same commit.

| Today | Goes to |
| --- | --- |
| `european_option` | 02, 04, 06 |
| `currency_forward_strip` | 06, and the strip in 07 |
| `validation_reporting` | 09 |
| `european_option_collars` | 06, 07 |
| `asian_forward_cma` | 02, 06 |
| `portfolio_validation` | 09, 10 |
| `portfolio_valuation` | 07 |
| `regulatory_capital` | 14 |
| `margining` | 07, 14 |
| `portfolio_paths` | 10, 12, 13, 14 |
| `sample_database` | 05 |
| `_collar_smoke.py` | Deleted once 07 asserts the same collar check |
| `README.md` | Rewritten as the index of the series |

## Gaps the notebooks will expose

Found while reading the code for this plan. Each is a place where a tutorial
would have to work around the library. Decide each one before the notebook
that meets it is written.

1. **No public way to simulate with netting sets without the deprecated
   class.** `simulate(..., by_netting_set=True)` wants an object with
   `portfolio` and `capital_model` attributes. `PortfolioB2CEM` is the only
   public one, and it warns. `python -m sjd` defines a private `_CapitalView`
   for the same purpose. Every strategy's `portfolio_b2cem` and the
   optimiser's `use_b2cem=True` go through `PortfolioB2CEM` too. Notebooks 14
   and 15 need this settled, and so does Step 22, which adds a second capital
   model. Either `simulate` takes `capital_model=`, or `sjd.capital` gains a
   public pair class.
2. **A `RatesEnvironment` can be read from Python.** `frs`, `drs`, `vrs`,
   `fixing_rs`, and `foreign_drs` return the curve in that slot, or `None`.
   `LoadedMarket` already carries the forward, the discount curve, and the
   volatility, so a catalogue market does not need a zero-strike contract to
   read its outright. `parse_book` accepts a `LoadedMarket`. Named schedules
   (`collar`, `forward`, `asian`) still need the sample market's date keys.
3. **A book file is read and never written.** `sjd.book` has `load_book` and
   `parse_book`. Nothing turns a `Portfolio` or a strategy back into TOML, so
   the markup round trip in notebooks 08 and 17 is one way.
4. **The pillar store can be written without SQL.** `save_market_pillars`,
   `open_market`, and `build_market` replace the hand-written inserts.
   `markets.store` still has the row readers. Notebook 05 can keep showing
   the tables; it no longer has to be the only writer.
5. **A `Simulation` and a `ValidationReport` do not pickle.** Notebook 17
   saves arrays and dicts instead. That is workable, and the notebook should
   say so plainly.
6. **Debug records cannot be shown.** The netting-set decisions that explain
   a capital number are `DEBUG`, and `DEBUG` is compiled out of the Release
   build a notebook runs on. Notebook 14 cannot show why two legs netted.
   Either those few records move to `INFO`, or the decisions are returned as
   data.
7. **The command line prices the sample market only.** Notebook 16 cannot run
   a catalogue market through `python -m sjd run`.
8. **Committed outputs.** nbmake runs the notebooks and does not rewrite the
   files, so committed outputs go stale again. Commit the new notebooks with
   outputs cleared, or add a check that they are.
9. **`is_valid` is true before the first `validate`.** It reads the last
   report, and an object that has never been validated has an empty one. The
   tutorials teach `validate` or `require_valid` first and never branch on
   `is_valid` alone. Whether an unvalidated object should read as valid is a
   library decision, not a notebook one.

Gap 1 changes what notebooks 14 and 15 look like. Gap 2 still blocks a
catalogue market from supplying the forward a book schedule needs. Notebook
08 uses the sample market, which already carries that forward. Notebook 05
prices a zero-strike forward for the same reason. The other gaps only change
a sentence.

## Phase 6

Step 22 ends with "a notebook comparing CEM and SA-CCR for outright, collar
and seagull strategies". That becomes `18_sa_ccr`, after 14 and 15:

- It reuses the three books and the CEM numbers from notebook 14.
- It needs the trade descriptors that Step 22 exposes first.
- Its validation, logging, and storage sections follow the same three
  headings, so the coverage tables above gain one column of entries and no new
  rows.
- Step 23 (risk weights and CVA capital) extends notebook 13's cost table and
  notebook 18. It does not need a notebook of its own.

## Order of work

01 to 11 are written, and `python/notebooks/README.md` indexes them. Notebook
05 prices a zero-strike forward. The environment now also returns its curves
(gap 2). Notebook 08 writes its own TOML and reads it back (gap 3).

1. Decide gap 1. Decide 8. Gap 2 is closed: the environment returns its curves.
2. Write 12 and 13. Notebooks 10 and 11 are written.
3. Write 14 and 15 once gap 1 is settled.
4. Write 16 and 17, then extend the notebook index.
5. Phase 6, with 18 as part of Step 22.
