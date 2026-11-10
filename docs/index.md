# sjd

Single-asset derivative pricing. A rates environment holds the curves.
Contracts check that environment, then price. Portfolios add margin.
Basel II current-exposure capital is a separate calculation. Python
simulates paths, summarises costs, and searches strategy parameters.

The [architecture](architecture.md) page is the map. [CEM capital](cem.md)
is the capital measure implemented today. [Error handling](error-handling.md)
and the [issue codes](issue-codes.md) describe validation. The
[Python API](api/python.md) is the front end, and the
[C++ API](api/cpp.md) is the Doxygen build of the headers.

Build the site from a configured preset, with the staged package on
`SJD_PYTHON_STAGE`:

```
cmake --preset debug
cmake --build --preset debug
```

```
SJD_PYTHON_STAGE=build/debug/python mkdocs build
doxygen docs/Doxyfile
```

The HTML site is `site/`. The C++ reference is `build/docs/doxygen/html`.
