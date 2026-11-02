# sjd

Single-asset derivative structuring library. A `RatesEnvironment` holds forward, discount, volatility, and fixing curves that share one anchor date. Contracts check that environment, then price. Portfolios group contracts and can add margin and Basel II current-exposure capital. The Python front end simulates paths, summarises exposure and valuation adjustments, and can revalue candidate strategies on a fixed set of spot paths for cost optimisation.

Option formulas stay unitless. They take a discount factor and a standard deviation, not an interest rate or a volatility with a day count, so Monte Carlo can call them without calendar logic.

## Scope: capital and costs

Regulatory capital today is Basel II Current Exposure Method (CEM). The C++ `Structure` / `CurrentExposureMethod` / `PortfolioB2CEM` path and the matching `sjd.stats.regulatory_capital` along a simulation are kept: CEM is a substantial, tested piece of the library and remains available for comparison and for books that still report under that measure.

For a structuring optimiser, the intended capital term is SA-CCR (Basel III). CEM add-ons are notional-based and ignore option delta, so they steer collar-versus-outright decisions the wrong way. SA-CCR for a single asset in one hedging set needs supervisory delta (Black-76 delta is already on the pricing formulas), adjusted notional, maturity factor, and a PFE multiplier from MtM and collateral — all of which can live in Python on `simulate(..., by_trade=True)` output without replacing the CEM code. Until that lands, `sjd.stats.structure_costs` uses CEM when netting-set capital is requested, and treats SA-CCR as the target measure to add beside it rather than instead of it.

Naming: a user-facing parametrised book is a **strategy** (`sjd.strategy`). C++ `Structure` and simulation `by_structure` still mean a regulatory-capital netting set.

## Dependencies

- C++17, CMake 3.24, Ninja
- [QuantLib](https://www.quantlib.org/) and Boost.Test, both from vcpkg (`x64-windows-static-md`)
- Python 3.10 and pybind11, when the bindings are built (the default)
- `tomli` on Python 3.10, for `python/markets/catalogue.toml` (Python 3.11 and later read it with the standard `tomllib`)

`VCPKG_ROOT` must be set. The debug preset passes `x64-windows-static-md` as the triplet.

## Build and test

```
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

`ctest` runs `sjdTest` (Boost) and `sjdPythonValidation`. Binaries land in `build/debug`. The Python module is also copied to `python/sjd_runtime` so notebooks do not import from the build tree.

Release uses the `release` preset. Set `-DSJD_BUILD_PYTHON=OFF` to skip the bindings.

## Layout

| Path | What it is |
| --- | --- |
| `include/sjd/contracts` | Contract ladder, portfolio, margin, CVA, regulatory capital |
| `include/sjd/rates` | `RatesEnvironment` and the discount, forward, volatility, and fixing families |
| `include/sjd/pricing` | Black76 and the Asian average-rate formula |
| `include/sjd/maths`, `include/sjd/tools` | Interpolation, dated curves, validation |
| `src/` | Sources, matching the public tree. Built by `src/CMakeLists.txt` |
| `tests/` | Boost tests, mirroring the public tree |
| `python/` | pybind11 module, `python/tests`, and notebooks in `python/notebooks` |

Public headers are included as `<sjd/contracts/...>`, `<sjd/rates/...>`, and so on.

## Valuation

Build the environment, validate, then price. `getValue` assumes `validate` has already succeeded. Rate lookups also assume the date was checked once at setup: Monte Carlo does not recheck every path.

```cpp
environment->setFRS(forward);
environment->setDRS(discount);
environment->setVRS(volatility);

contract.validate(environment);
if (contract.isOK()) {
    double value = contract.getValue(environment);
}
```

From Python the same steps are `validate`, `is_ok`, and `get_value`. See `python/notebooks` for worked examples. Build the bindings so `_core` is copied into `python/sjd`, then `pip install -e python` and `import sjd`.

CMake is the supported build. The old Excel add-in layers (`sjdObjects`, `sjdXllObjects`, and the Visual Studio solutions) have been removed.
