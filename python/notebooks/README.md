# Teaching notebooks

Build the compiled core, which is copied into the package as `python/sjd/_core.pyd`,
and install the package in editable mode once:

```
cmake --build --preset debug --target sjd_python
python -m pip install -e python
```

After that `import sjd` works from any directory, and a rebuild is picked up
without reinstalling. Shut the notebook kernel down before rebuilding so Windows
can replace `_core.pyd`.

The sample market and book come from `sjd.examples` (`sample_market`,
`sample_contracts`), so every notebook works on the same trades.

The running example, from the forward strip onward, is a single FX-style
commodity curve: anchor 16 September 2026, spot 17.50, domestic rate 7%,
foreign rate 4%, volatility 12%. The portfolio notebooks put a collar, an
outright forward, and an Asian forward on that curve, then compare the book
with and without a variation-margin CSA under Basel II current exposure.

| Order | Notebook | What it shows |
| --- | --- | --- |
| 1 | [european_option.ipynb](european_option.ipynb) | Curves, a rates environment, and one European option |
| 2 | [currency_forward_strip.ipynb](currency_forward_strip.ipynb) | A strip of physically settled forwards |
| 3 | [validation_reporting.ipynb](validation_reporting.ipynb) | Structured validation reports |
| 4 | [european_option_collars.ipynb](european_option_collars.ipynb) | Monthly collars checked against Black-76 |
| 5 | [asian_forward_cma.ipynb](asian_forward_cma.ipynb) | Calendar-month-average Asian forwards |
| 6 | [portfolio_validation.ipynb](portfolio_validation.ipynb) | Portfolio reports, and why a deterministic forward cannot drive a path |
| 7 | [portfolio_valuation.ipynb](portfolio_valuation.ipynb) | The sample book and portfolio mark-to-market |
| 8 | [regulatory_capital.ipynb](regulatory_capital.ipynb) | Unmargined Basel II current exposure |
| 9 | [margining.ipynb](margining.ipynb) | A CSA, and how posted variation margin changes capital |
| 10 | [portfolio_paths.ipynb](portfolio_paths.ipynb) | Spot-path and Monte Carlo profiles with and without margin |

`_collar_smoke.py` checks the collar strip. It is not part of the reading order.

Only Basel II current exposure is implemented. The margin model is variation
margin: thresholds, a minimum transfer amount, and rounding. Initial margin,
SA-CCR, SIMM, and CVA are not in this set.
