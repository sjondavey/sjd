# Python front end

Step 20. A book file, a report folder, and a command line replace the Excel
front end. Strategies and the parameter search stay in Python. The C++
library is unchanged.

## Book file

A book is TOML, the same format as the market catalogue. JSON and CSV are
not accepted. Dates are TOML dates or `YYYY-MM-DD` strings.

`sjd.book.parse_book` reads the text. `load_book` reads a path. Both return
a `Book` whose `contracts()` are `sjd` instruments. A bad file raises
`BookError`. The message names the trade index and the field.

`[book]` may set `name` and a default `volume`. Each `[[trades]]` entry has
a `kind`:

| kind | Contracts |
| --- | --- |
| `cashflow` | one `FixedCashflow` |
| `european_forward` | one physically settled `EuropeanForward` |
| `european_option` | one `EuropeanOption` |
| `asian_forward` | one `AsianForward` |
| `asian_option` | one `AsianOption` |
| `collar` | `CollarStrategy` |
| `strip` | `StripStrategy` |
| `seagull` | `SeagullStrategy` |
| `participating_forward` | `ParticipatingForwardStrategy` |

`buy_sell` is `BUY` or `SELL` and is the bank's side. `client` on a collar,
seagull, or participating forward is `producer` or `consumer`; omitted, it is
`producer`. `type` is `PUT` or `CALL`. An omitted premium
date is the settlement date, and an omitted premium amount is zero.

A strike is either absolute or a moneyness. Moneyness is
`forward * moneyness`, rounded to two decimals, using the market forward on
the settlement date. An Asian strike uses the average of the forwards on the
averaging dates. Loading a moneyness without a market is an error.

`schedule` names dates on the market instead of writing them out:

- `collar` uses `collar_maturity` and `collar_settlement`
- `forward` uses `forward_settlement` for both maturity and settlement
- `asian` uses `averaging_dates` and `asian_settlement`

Explicit dates replace the schedule. `python/sjd/books/sample.toml` is the
sample book: a collar, a forward, and an Asian, with the same moneyness as
`examples.sample_contracts`.

An optional `[margin]` table builds a `SimpleMargin` on the market anchor
from the variation-margin fields the constructor already takes. Initial
margin is not in the file.

## Market

`python -m sjd run` accepts `--market sample` only. That is
`examples.sample_market(stochastic=True)`, because a path run needs a
forward that can roll. A catalogue quote needs a pillar database and is not
part of this command.

## Reports

`sjd.reports` builds pandas DataFrames:

- `prices` is one row per contract: kind, settlement, volume, buy/sell, value.
  A contract with a strike adds that column. An option adds `put_call`.
- `exposure_profile` is one row per simulation date: expected exposure,
  negative expected exposure, and mean mark-to-market
- `costs_table` is one row of `structure_costs`
- `capital_table` is one row per netting set from a `CapitalResult`

`write_excel` writes one sheet per frame. It needs openpyxl. Without
pandas, these functions raise `ImportError` naming the `front` extra.
The core install does not depend on pandas, scipy, or openpyxl.

## Command line

```
python -m sjd run book.toml --market sample --paths 10000 --output report
```

The command loads the book, prices it, and simulates `--paths` paths on the
market's simulation dates with the given `--seed` (default 1). Paths are
stored, not aggregated, so CVA is the mean of the pathwise adjustments.
Costs use a flat default probability of 0.02 and a loss given default of
0.6 unless the flags say otherwise. Capital uses commodity supervisory
factors, risk weight 1, and capital ratio 0.08. `--no-capital` skips that
sheet.

The folder receives `prices.csv`, `exposure.csv`, `costs.csv`,
`capital.csv` when capital was requested, and `report.xlsx` when openpyxl
is installed. The command prints the folder path.

## Notebooks

The linux-release CI job runs `pytest --nbmake python/notebooks` against
the staged package, with `MPLBACKEND=Agg`. One job is enough. The notebooks
are short. `nbmake`, `matplotlib`, pandas, openpyxl, scipy, and QuantLib
are installed there with the other Python tools, so mypy can import them.
pandas, openpyxl, scipy, and QuantLib stay untyped: the stub packages are
not installed. `xau_usd_rates.ipynb` writes the local database, and
`xau_usd_3y.ipynb` is collected immediately after it.

## Strategies

A strategy prices the bank's side of the trade. `client` is `producer` or
`consumer`. A producer sells the commodity, so the bank sells the put and
buys the call. A consumer buys the commodity, so the bank buys the put and
sells the call. The default is `producer`. A book file sets the same label
with `client`.

`StripStrategy` is one physically settled forward per settlement, all with
the same buy/sell and volume. `buy_sell` is the bank's side: `BUY` is the
bank buying from a producer. Strikes are a single number, reused, or one
per settlement. Settlements are strictly increasing. `from_moneyness`
strikes each settlement off its own forward.

`SeagullStrategy` is a collar plus a lower put. Against a producer the bank
sells the put, buys the call, and buys the lower put. The strikes satisfy
`floor < put < call`.

`ParticipatingForwardStrategy` trades a put on the full volume and a call on
`volume * (1 - participation)`. Against a producer the bank sells the put
and buys the call. `participation` is the fraction of upside the client
keeps and lies in `(0, 1)`. The put strike is below the call strike.

Each of these has `contracts`, `portfolio`, `portfolio_b2cem`, and
`premium`. `premium` is the bank's option value, with sold options negative.
A strip has no options, so its premium is zero.

## Strips

`sjd.structuring` builds the same strategies across a schedule.
`LoadedMarket.monthly_schedule(start, months)` is one settlement a month,
adjusted on the quote's calendar. A point quote also stores the option
expiry. A calendar-month quote stores the averaging dates and the cash
settlement. `step_ladder(start, step, count)` repeats a strike for twelve
periods, then steps.

`ForwardStrip`, `CollarStrip`, `SeagullStrip`, and `ParticipatingForwardStrip`
take a scalar or one value per period. Each strip keeps its legs grouped by
settlement and exposes `contracts`, `portfolio`, `pv`, `period_pvs`, and
`payoff`. The same names are available from `sjd.strategy`.

`solve_pv(build, market, target_pv)` finds the one number `build` takes so
the bank's present value equals the target. It checks the two ends of the
bracket, then uses `scipy.optimize.brentq`. `compare_structures` returns a
group summary and a settlement table. `plot_structures` draws the outright,
the strikes, and the combined payoff. `sjd.optimise.solve_pv` is the same
function. `minimise_strategy` remains the search on simulated paths.

## Rates

`sjd.markets.strip` turns deposit quotes, GOFO quotes, and broker smiles
into pillars. It needs the `rates` extra, which is QuantLib.
`strip_usd_deposits` builds the USD curve. `strip_gofo_to_lease` turns a
GOFO simple rate into the lease rate the catalogue stores, and adds the gold
spot date and the value dates to the discount pillars.
`delta_pillars_from_rr_bf` stores the smile as put deltas, including the
anchor. `markets.save_market_pillars` writes those pillars onto a rates
label. The label is a name plus the datetime it was saved, and it has one
anchor date. A different anchor on a later instrument is refused. Another
commodity uses the same label. `markets.open_market(..., label="close")`
reads the newest instance of that name, unless `as_of` names an older one.
`markets.build_market` does the same construction in memory.

A new commodity is a catalogue entry. `load_catalogue(path, base=CATALOGUE)`
keeps the shipped calendars and adds the new market. `kind = "flat"` is one
volatility per date. `kind = "moneyness"` is a point moneyness surface.
`kind = "gofo_rate"` stores the GOFO simple rate. `kind = "lease_rate"` is
the same as `gofo`. `kind = "cross"` is one outright times another.
`metal_times_usdzar` stays as it was.

## Search

`minimise_collar_put_moneyness` stays the one-dimensional grid.

`minimise_strategy` is the general search. The caller draws spots once and
passes them in, so every trial uses the same paths. `build(parameters)`
returns a strategy. The score is `structure_costs[objective]` plus
`premium_weight * premium`, the same combination as the collar grid.
`scipy.optimize.minimize` runs the search. The default method is
`L-BFGS-B`, which honours `bounds`. A missing scipy raises `ImportError`
naming the `front` extra.

The result has `strategy`, `parameters`, `costs`, `premium`, `score`, and
the scipy `result`.
