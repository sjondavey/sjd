"""Monte Carlo statistics for an ``sjd.simulate`` result, using numpy only.

The functions take a ``[paths, steps]`` array, such as ``simulation.mtm``,
``exposure(simulation)`` or one trade's slice ``simulation.by_trade[:, :, k]``.
They return one value per simulation date.

Quantiles use ``np.quantile`` and accept its ``method``. The C++ evolver's
``PFE`` column takes the sorted value at index ``int(paths * level)``, so it can
differ from these quantiles by one order statistic.
"""

import numpy as np


def exposure(simulation):
    """MtM less the margin balance on the same date, for every simulated date.

    Without margin this is a copy of the MtM.
    """
    mtm = np.asarray(simulation.mtm, dtype=float)
    if simulation.margin is None:
        return mtm.copy()
    return mtm - np.asarray(simulation.margin, dtype=float)


def holding_period_exposure(simulation):
    """MtM on each requested date less the margin from its margin reset date.

    The result is shaped ``[paths, len(exposure_dates(simulation))]``. It
    matches the evolver's "with HP" columns. Without a holding period the reset
    date is the exposure date itself.
    """
    mtm = np.asarray(simulation.mtm, dtype=float)[:, simulation.exposure_steps]
    if simulation.margin is None:
        return mtm
    return mtm - np.asarray(simulation.margin, dtype=float)[:, simulation.margin_reset_steps]


def exposure_dates(simulation):
    """The requested simulation dates, without the added margin reset dates."""
    return [simulation.dates[step] for step in simulation.exposure_steps]


def expected_exposure(values):
    """Mean of the positive part of each column."""
    return np.maximum(_paths(values), 0.0).mean(axis=0)


def negative_expected_exposure(values):
    """Mean of the negative part of each column. The result is zero or below."""
    return np.minimum(_paths(values), 0.0).mean(axis=0)


def potential_future_exposure(values, level=0.95, method="linear"):
    """The ``level`` quantile of the positive part of each column."""
    _check_level(level)
    return np.quantile(np.maximum(_paths(values), 0.0), level, axis=0, method=method)


def negative_potential_future_exposure(values, level=0.95, method="linear"):
    """The ``1 - level`` quantile of the negative part of each column."""
    _check_level(level)
    return np.quantile(np.minimum(_paths(values), 0.0), 1.0 - level, axis=0, method=method)


def quantiles(values, levels, method="linear"):
    """Quantiles of each column, shaped ``[len(levels), steps]``."""
    return np.quantile(_paths(values), np.asarray(levels, dtype=float), axis=0, method=method)


def value_at_risk(values, step, level=0.99, method="linear"):
    """Loss not exceeded with probability ``level`` between step 0 and ``step``.

    The loss is the fall in value. Cash from trades that settle before
    ``step`` is not counted, because a settled trade has no value on the path.
    """
    _check_level(level)
    return -np.quantile(_change(values, step), 1.0 - level, method=method)


def expected_shortfall(values, step, level=0.99, method="linear"):
    """Mean loss over the paths whose loss is at least ``value_at_risk``."""
    change = _change(values, step)
    threshold = -value_at_risk(values, step, level, method)
    return -change[change <= threshold].mean()


def standard_error(values, antithetic=False):
    """Standard error of each column mean.

    With antithetic paths, path ``i`` and path ``i + paths // 2`` are not
    independent. The error then uses the mean of each pair as one sample.
    """
    values = _paths(values)
    if antithetic:
        if values.shape[0] % 2:
            raise ValueError("antithetic values need an even number of paths")
        half = values.shape[0] // 2
        values = 0.5 * (values[:half] + values[half:])
    if values.shape[0] < 2:
        raise ValueError("standard error needs at least two independent samples")
    return values.std(axis=0, ddof=1) / np.sqrt(values.shape[0])


def summary(simulation, level=0.95, holding_period=False):
    """Statistics for each requested simulation date, as a dict of equal-length columns.

    The keys are ``dates``, ``spot``, ``mtm``, ``margin`` (only when the
    portfolio is margined), ``ee``, ``nee``, ``pfe``, ``negative_pfe`` and
    ``ee_standard_error``. With ``holding_period``, the exposures use
    ``holding_period_exposure``. ``pd.DataFrame(summary(simulation))`` gives a
    table.
    """
    steps = simulation.exposure_steps
    if holding_period:
        values = holding_period_exposure(simulation)
    else:
        values = exposure(simulation)[:, steps]
    table = {
        "dates": exposure_dates(simulation),
        "spot": np.asarray(simulation.spot)[:, steps].mean(axis=0),
        "mtm": np.asarray(simulation.mtm)[:, steps].mean(axis=0),
    }
    if simulation.margin is not None:
        table["margin"] = np.asarray(simulation.margin)[:, steps].mean(axis=0)
    table["ee"] = expected_exposure(values)
    table["nee"] = negative_expected_exposure(values)
    table["pfe"] = potential_future_exposure(values, level)
    table["negative_pfe"] = negative_potential_future_exposure(values, level)
    table["ee_standard_error"] = standard_error(np.maximum(values, 0.0), simulation.antithetic)
    return table


def _paths(values):
    values = np.asarray(values, dtype=float)
    if values.ndim != 2:
        raise ValueError("values must be shaped [paths, steps]")
    return values


def _change(values, step):
    values = _paths(values)
    if not 0 < step < values.shape[1]:
        raise ValueError("step must be after the first simulation date")
    return values[:, step] - values[:, 0]


def _check_level(level):
    if not 0.0 < level < 1.0:
        raise ValueError("level must be between 0 and 1")
