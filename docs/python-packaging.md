# Python packaging

Step 18. `pip install .` and `pip install -e .` compile `_core`. The source tree
does not receive a copy of the extension. The version is the CMake project
version. Wheels are a separate CI job. Dates accept `numpy.datetime64` and
`pandas.Timestamp`.

## Build

`pyproject.toml` is at the repository root, because that is where
`CMakeLists.txt` is and where `pip install .` and cibuildwheel look. The
setuptools file under `python/` is removed. scikit-build-core is the build
backend. It drives this CMake build. The wheel build type is Release, and
`SJD_BUILD_TESTS` is off. The bindings stay on.

`python/sjd` is the pure-Python package. scikit-build finds it at
`python/<project name>`. The extension is installed with the `python`
component:

```
install(TARGETS sjd_python LIBRARY DESTINATION sjd RUNTIME DESTINATION sjd COMPONENT python)
```

`install.components` is `["python"]`, so the static library and the CMake
package config stay out of the wheel. On Windows the `.pyd` is a module.
CMake applies `COMPONENT` only to the artifact group it follows, so both the
library and the runtime groups name the `python` component and both
destinations are `sjd`.

An editable install uses scikit-build's default redirect. The CMake install
tree is first on the package path and holds `_core` and `_core.pyi`. The
source tree holds the `.py` files. `import sjd` loads the extension from the
install tree. A C++ change is picked up by running `pip install -e .` again.
The preset build is a different tree, and it stays the way `ctest` is run.

`pip install .` does not use a preset. Before `project()`, if
`CMAKE_TOOLCHAIN_FILE` is unset and `VCPKG_ROOT` is set, CMake uses that
vcpkg toolchain. If `VCPKG_TARGET_TRIPLET` is unset, Windows defaults to
`x64-windows-static-md` and Unix to `x64-linux`. Those are the host checks
(`CMAKE_HOST_WIN32`, `CMAKE_HOST_UNIX`), because `WIN32` is not set until
`project()`. A preset already sets both variables, so the preset wins. The
default vcpkg triplet `x64-windows` is not used.

## Staging

`import sjd` loads `_core` immediately, so the extension has to sit inside
the `sjd` package Python finds. Preset builds (anything that is not a
scikit-build build) stage that package at `${CMAKE_BINARY_DIR}/python/sjd`:
a copy of `python/sjd`, the built module, and the generated stub. `sjdPython`
sets `PYTHONPATH` to `${CMAKE_BINARY_DIR}/python`. The stage is refreshed by
the default build, including when only a `.py` file changed.

There is no `POST_BUILD` copy into `python/sjd`. A leftover `_core` in the
source tree would shadow an import and is deleted.

## Version

`project(sjd VERSION 0.1.0 ...)` is the only version. The wheel version is
dynamic: scikit-build-core's regex provider reads that `project()` line.
`sjd.__version__` and `build_info()["version"]` already come from
`SJD_VERSION`, which is `PROJECT_VERSION`.

## Stubs and checks

`python/generate_core_stub.py` runs `pybind11-stubgen` after the module is
importable and writes `sjd/_core.pyi` into the stage. The working directory
is the repository root, so a binary sitting in the build directory is not
picked up as `sjd`. Enum defaults in the pybind11 docstrings are mapped to
`sjd._core`. The two spot-path methods return `Simulation`, whose C++ type
is in an anonymous namespace; the script replaces that annotation.
`ValidationError.report` is a property so the stub exposes it. A parameter
written by pybind11 as `T = None` is rewritten to `T | None = None`, which is
what the default means.
The wheel installs the stub next to the extension. The stub is not committed.
`python/sjd/py.typed` is an empty marker shipped with the package.

`pytest`, `ruff`, and `mypy` are configured in the root `pyproject.toml`.
`testpaths` is `python/tests`. Ruff's `src` is `python`, so `sjd` stays a
first-party import, and `python/notebooks` stays out of the check. `mypy` is
the default checker, not `--strict`. It runs in CI against the staged
package, which has the stub. The star import in `sjd/__init__.py` needs the
stub's `__all__`. `set_log_level` in that file replaces the imported function,
so the redefinition is ignored by mypy.

## Wheels

cibuildwheel is its own CI job, not a cell of the preset matrix. It builds
CPython 3.10 only: `cp310-win_amd64` and `cp310-manylinux_x86_64`. macOS,
musllinux, other CPython versions, and 32-bit and ARM tags are skipped. There
is no macOS preset or triplet.

The job checks out the same vcpkg commit the other jobs use
(`6fa128f2b6e1dd260ebfb30fdb1c38a191369812`), inside the build environment.
On Linux that environment is the manylinux container, so the clone happens
there rather than on the host. `VCPKG_ROOT` points at that clone. QuantLib is
built again for the wheel. The installed wheel is what the tests import.
`pytest` is pointed at `{project}/python/tests`, and the source tree is not
put ahead of the install on `PYTHONPATH`.

## Dates

`to_ql_date` still reads `year`, `month`, and `day`. That covers
`datetime.date`, `datetime.datetime`, and `pandas.Timestamp` without
importing pandas. pandas is not a dependency.

`numpy.datetime64` is a separate path. `NaT` is rejected. Any other value is
cast with `astype("datetime64[D]")`, read as an `int64` count of days since
1970-01-01, and added to `QuantLib::Date(1, January, 1970)`. A finer unit
uses that UTC day. The numpy type is cached as a borrowed reference that outlives the
interpreter, so a later `datetime.date` does not import numpy again. A
pybind11 object in that cache is destroyed during finalisation and crashes
the process. `pandas.NaT` has no usable `year` and raises a value
error. A value with none of these shapes raises a type error.
