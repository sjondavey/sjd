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

`buy_sell` is `BUY` or `SELL`. `type` is `PUT` or `CALL`. An omitted premium
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

- `prices` is one row per contract: kind, settlement, volume, buy/sell, value
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
are short. `nbmake`, `matplotlib`, pandas, openpyxl, and scipy are
installed there with the other Python tools, so mypy can import them.
Those three stay untyped: the stub packages are not installed.

## Strategies

`CollarStrategy` is unchanged.

`StripStrategy` is one physically settled forward per settlement, all with
the same buy/sell and volume. Strikes are a single number, reused, or one
per settlement. Settlements are strictly increasing. `from_moneyness`
strikes each settlement off its own forward.

`SeagullStrategy` is a producer seagull on one maturity: buy a put, sell a
call, and sell a lower put, all on the same volume. The strikes satisfy
`floor < put < call`.

`ParticipatingForwardStrategy` buys a put on the full volume and sells a
call on `volume * (1 - participation)`. `participation` is the fraction of
upside kept and lies in `(0, 1)`. The put strike is below the call strike.

Each of these has `contracts`, `portfolio`, `portfolio_b2cem`, and
`premium`. `premium` is the sum of `get_option_value`. A strip has no
options, so its premium is zero.

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
