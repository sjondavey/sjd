"""Commodity derivative pricing and Monte Carlo statistics.

The compiled library is ``sjd._core``; its classes and functions are re-exported
here, so ``sjd.EuropeanOption`` and ``sjd.simulate`` work directly.

Subpackages and modules, imported on first use:

- ``sjd.markets``: market catalogue, pillar store and rates environments.
- ``sjd.stats``: exposure, CVA, DVA, funding, CEM capital and structure costs.
- ``sjd.strategy``: parametrised books (collar, strip, seagull, participating forward).
- ``sjd.structuring``: schedules, multi-period strips, and a present-value solve.
- ``sjd.optimise``: revalue strategies on fixed spot paths and search parameters.
- ``sjd.book``: load a TOML book into contracts. ``python -m sjd run`` prices one.
- ``sjd.reports``: pandas tables for prices, exposure, costs and capital.
- ``sjd.examples``: the sample market and book used by the notebooks and tests.

``SjdError`` is the base exception. ``ConfigurationError`` and ``ValidationError``
are also ``ValueError``. ``NumericalError`` is also ``ArithmeticError``.
``require_valid`` runs ``validate`` and raises ``ValidationError`` when the
report contains an error. ``set_log_level`` sets the C++ logger and the
``sjd`` logger together. Pass ``dedupe_warnings=True`` to show each C++
warning once. C++ records are also written to ``sjd._core``, so
``logging.basicConfig(level=logging.INFO)`` shows simulation and structuring.
``__version__`` and ``build_info`` come from the CMake project version and the
git commit at configure time. ``simulation.seed`` and ``simulation.metadata``
record how a Monte Carlo result was produced.
"""

import logging

import sjd._core as _core
from sjd._core import *  # noqa: F401,F403
from sjd.capital import PortfolioB2CEM as PortfolioB2CEM  # noqa: F401

logging.addLevelName(5, "TRACE")

__version__ = _core.__version__


def require_valid(obj, env=None):
    """Validate ``obj`` and raise ``ValidationError`` when the report has an error.

    ``env`` is passed through when the object checks itself against a rates
    environment. Returns the report.
    """
    report = obj.validate() if env is None else obj.validate(env)
    report.raise_if_invalid()
    return report


class _WarningDedupe(logging.Filter):
    """Show each warning from ``sjd._core`` once until the filter is removed."""

    def __init__(self):
        super().__init__()
        self.seen = set()

    def filter(self, record):
        if record.levelno < logging.WARNING:
            return True
        key = (record.name, record.levelno, record.getMessage())
        if key in self.seen:
            return False
        self.seen.add(key)
        return True


_warning_dedupe = _WarningDedupe()
_dedupe_warnings = False


def set_log_level(level, dedupe_warnings=False):  # type: ignore[no-redef]
    """Set the C++ logger and the ``sjd`` logger to the same level.

    ``level`` is ``"TRACE"``, ``"DEBUG"``, ``"INFO"``, ``"WARNING"`` or
    ``"ERROR"``, or a ``logging`` number. ``DEBUG`` and ``TRACE`` are compiled
    out of a Release build. Simulation and CEM optimisation are ``INFO``.
    ``dedupe_warnings=True`` keeps the first copy of each ``sjd._core`` warning
    and drops the repeats. Calling it again does not add a second filter.
    ``dedupe_warnings=False`` removes that filter.
    """
    global _dedupe_warnings
    if isinstance(level, int):
        name = logging.getLevelName(level)
        logging.getLogger("sjd").setLevel(level)
    else:
        name = str(level).upper()
        if name == "WARN":
            name = "WARNING"
        if name == "OFF":
            logging.getLogger("sjd").setLevel(logging.CRITICAL + 1)
        else:
            logging.getLogger("sjd").setLevel(name)
    _core.set_log_level(name)
    core = logging.getLogger("sjd._core")
    if dedupe_warnings:
        if not _dedupe_warnings:
            _warning_dedupe.seen.clear()
            core.addFilter(_warning_dedupe)
            _dedupe_warnings = True
    elif _dedupe_warnings:
        core.removeFilter(_warning_dedupe)
        _dedupe_warnings = False
