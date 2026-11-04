"""Commodity derivative pricing and Monte Carlo statistics.

The compiled library is ``sjd._core``; its classes and functions are re-exported
here, so ``sjd.EuropeanOption`` and ``sjd.simulate`` work directly.

Subpackages and modules, imported on first use:

- ``sjd.markets``: market catalogue, pillar store and rates environments.
- ``sjd.stats``: exposure, CVA, DVA, funding, CEM capital and structure costs.
- ``sjd.strategy``: parametrised books (for example a collar) that build contracts.
- ``sjd.optimise``: revalue strategies on fixed spot paths and search parameters.
- ``sjd.examples``: the sample market and book used by the notebooks and tests.

``SjdError`` is the base exception. ``ConfigurationError`` and ``ValidationError``
are also ``ValueError``. ``NumericalError`` is also ``ArithmeticError``.
``require_valid`` runs ``validate`` and raises ``ValidationError`` when the
report contains an error.
"""

from sjd._core import *  # noqa: F401,F403


def require_valid(obj, env=None):
    """Validate ``obj`` and raise ``ValidationError`` when the report has an error.

    ``env`` is passed through when the object checks itself against a rates
    environment. Returns the report.
    """
    report = obj.validate() if env is None else obj.validate(env)
    report.raise_if_invalid()
    return report
