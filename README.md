# sjd

Single-asset valuation library. A `RatesEnvironment` holds forward, discount, volatility, and fixing curves that share one anchor date. Contracts check that environment, then price. Portfolios group contracts and can add margin, CVA, insurance, and Basel II current exposure.

Option formulas stay unitless. They take a discount factor and a standard deviation, not an interest rate or a volatility with a day count, so Monte Carlo can call them without calendar logic.

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

From Python the same steps are `validate`, `is_ok`, and `get_value`. See `python/notebooks` for worked examples. The module is `sjd`, loaded from `python/sjd_runtime`.

## What is not the build

CMake is the supported build, and `python/` is the supported front end. `excelFunctions` and `xllInterface` have been removed. `sjd_vc10.sln`, `sjd_vc14.sln`, `sjdObjects/`, and `sjdXllObjects/` are not inputs to CMake and are not maintained.
