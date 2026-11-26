# Teaching notebooks

From the repository root, install the package and the packages these notebooks plot with:

```
python -m pip install -e ".[front]" matplotlib
```

`front` brings pandas, which the notebooks use for `Timestamp` and tables. matplotlib is what the curve and smile charts use. After that, `import sjd` works from any directory. This is a Release build, separate from `cmake --build --preset debug`, which is what `ctest` runs. A later C++ change is picked up by installing again. Shut the notebook kernel down before that so Windows can replace the extension.

The market-rate notebooks, the trade notebooks, and the simulation notebooks are written. Each one turns logging on, checks its claims, and writes only to a temporary directory. Statistics onward is described in `docs/notebook-plan.md` and is not written yet.

`xau_usd_rates.ipynb` declares USD deposits, GOFO, and a volatility smile, then calls `sjd.markets.strip` and `save_market_pillars` under the rates label `close`. That notebook needs the `rates` extra (`pip install QuantLib`). `xau_usd_3y.ipynb` opens that label and solves a forward strip, a collar strip, and a seagull with `sjd.structuring`. They are working notebooks beside the numbered series.

The running example is one FX-style curve: anchor 16 September 2026, spot 17.50, domestic rate 7%, foreign rate 4%, volatility 12%. Notebooks 02 to 04 build it by hand. Notebook 05 builds the catalogue markets from stored pillars. From notebook 06 the market is `examples.sample_market()`, and the sample book is the collar, forward, and Asian in `examples.sample_contracts`.

| Order | Notebook | What it shows |
| --- | --- | --- |
| 1 | [01_getting_started.ipynb](01_getting_started.ipynb) | The install, dates, calendars, and logging |
| 2 | [02_curves.ipynb](02_curves.ipynb) | Discount, forward, volatility, and fixing curves |
| 3 | [03_volatility_surfaces.ipynb](03_volatility_surfaces.ipynb) | Delta and moneyness surfaces |
| 4 | [04_rates_environment.ipynb](04_rates_environment.ipynb) | One anchor date and its five curves, including FX |
| 5 | [05_market_catalogue_and_store.ipynb](05_market_catalogue_and_store.ipynb) | Market conventions in TOML and pillars under a rates label |
| 6 | [06_contracts.ipynb](06_contracts.ipynb) | Forwards, options, Asians, and cashflows |
| 7 | [07_portfolios_and_strategies.ipynb](07_portfolios_and_strategies.ipynb) | Portfolios, the four strategies, and margin |
| 8 | [08_book_files.ipynb](08_book_files.ipynb) | The same portfolios as TOML books |
| 9 | [09_validation_and_errors.ipynb](09_validation_and_errors.ipynb) | Reports, exceptions, and the simulation gate |
| 10 | [10_simulation.ipynb](10_simulation.ipynb) | Paths, spot, mark-to-market, and the simulation options |
| 11 | [11_logging_and_provenance.ipynb](11_logging_and_provenance.ipynb) | Log records, and the seed and metadata of a run |

`_collar_smoke.py` checks a collar strip. It is not part of the reading order.
