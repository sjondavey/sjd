# C++ API

The public headers are `include/sjd`. Include them as
`<sjd/contracts/...>`, `<sjd/rates/...>`, `<sjd/pricing/...>`,
`<sjd/maths/...>`, and `<sjd/tools/...>`.

Doxygen builds the reference from those headers. Run it from the
repository root:

```
doxygen docs/Doxyfile
```

The HTML is `build/docs/doxygen/html/index.html`. Every public member is
listed. A `/** */` comment on a type or a function is the description.
Members without a comment are still listed.

QuantLib headers are included by the rates and contract headers. Point
`INCLUDE_PATH` in the Doxyfile at the vcpkg include directory if those
warnings get in the way. They do not stop the build.

The Python names for these types are on the [bindings](bindings.md) page.
