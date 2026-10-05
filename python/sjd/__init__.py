"""Commodity derivative pricing and Monte Carlo statistics.

The compiled library is ``sjd._core``; its classes and functions are re-exported
here, so ``sjd.EuropeanOption`` and ``sjd.simulate`` work directly.

Subpackages, imported on first use:

- ``sjd.markets``: market catalogue, pillar store and rates environments.
- ``sjd.stats``: exposure, CVA, DVA, funding and regulatory capital for an ``sjd.simulate`` result.
- ``sjd.examples``: the sample market and book used by the notebooks and tests.
"""

from sjd._core import *  # noqa: F401,F403
