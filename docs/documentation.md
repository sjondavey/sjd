# Documentation

Step 21. The site is MkDocs. The C++ reference is Doxygen. Neither is a
required install of the library.

## Site

`mkdocs.yml` builds the pages under `docs/` into `site/`. The nav is the
architecture page, the CEM methodology, `error-handling.md`,
`issue-codes.md`, the Python API, the binding index, and a short C++ page.
The step design notes and `MODERNISATION_TODO.md` are excluded. They are
the record of how the code changed, not the user guide.

`mkdocstrings` renders the pure-Python modules: `strategy`, `book`,
`reports`, `stats`, `optimise`, `examples`, `capital`, and `markets`, plus
`require_valid` and `set_log_level`. Private names are omitted. The bound
classes are not passed through mkdocstrings. A build hook writes
`docs/api/bindings.md` from `__all__` and the class docstrings in the
generated `_core.pyi`. That file is not committed.

The build needs the staged package. `SJD_PYTHON_STAGE` points at
`build/<preset>/python`, and the hook puts that directory on `sys.path`
before the Python modules are imported. Without it, `import sjd` has no
`_core`.

## C++ reference

`docs/Doxyfile` reads `include/sjd` and writes HTML under
`build/docs/doxygen`. Every public member is extracted. Existing `/** */`
comments are the text. Missing comments do not fail the build. Doxygen is
run from the repository root.

## Issue codes

`docs/issue-codes.md` stays the table of codes and meanings.
`docs/generate_issue_codes.py --check` fails when a constant in
`include/sjd/tools/IssueCodes.h` is missing from the table, or the table
names a code the header does not define. `--write` rewrites the table and
keeps the meanings. A unit test runs the check. The error-handling page is
unchanged.

## CI

The linux-release job builds the site and the Doxygen HTML after mypy.
One job is enough. `mkdocs` and `mkdocstrings` are dev dependencies, not
library dependencies. `pymdown-extensions` stays below 10.15: later
releases require a Markdown instance in `Highlight`, and mkdocstrings 1.0
does not pass one. Doxygen is installed in that job.
