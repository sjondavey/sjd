"""Commodity derivative pricing and Monte Carlo statistics.

The compiled library is ``sjd._core``; its classes and functions are re-exported
here, so ``sjd.EuropeanOption`` and ``sjd.simulate`` work directly.

Subpackages and modules, imported on first use:

- ``sjd.markets``: market catalogue, pillar store and rates environments.
- ``sjd.stats``: exposure, CVA, DVA, funding, CEM capital and structure costs.
- ``sjd.strategy``: parametrised books (for example a collar) that build contracts.
- ``sjd.optimise``: revalue strategies on fixed spot paths and search parameters.
- ``sjd.examples``: the sample market and book used by the notebooks and tests.
"""

from sjd._core import *  # noqa: F401,F403
