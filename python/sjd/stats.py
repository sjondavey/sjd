"""Monte Carlo statistics for an ``sjd.simulate`` result, using numpy only.

The functions take a ``[paths, steps]`` array, such as ``simulation.mtm``,
``exposure(simulation)`` or one trade's slice ``simulation.by_trade[:, :, k]``.
They return one value per simulation date.

Quantiles use ``np.quantile`` and accept its ``method``.

Credit and debit valuation adjustments, cost of funding, and regulatory capital
along a path are computed here. The simulation itself stays in C++.
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


def flat_default_probability(anchor, dates, annual_probability):
    """Cumulative default probability of a flat annual conditional rate.

    Matches ``FlatDefaultProbability``: the hazard is ``-log(1 - annual_probability)``
    and the year fraction is calendar days divided by 365. The anchor date is 0.
    """
    if not 0.0 <= annual_probability <= 1.0:
        raise ValueError("annual_probability must be between 0 and 1")
    years = np.array([(day - anchor).days / 365.0 for day in dates], dtype=float)
    if annual_probability == 1.0:
        return np.where(years > 0.0, 1.0, 0.0)
    hazard = -np.log(1.0 - annual_probability)
    cumulative = 1.0 - np.exp(-np.maximum(years, 0.0) * hazard)
    return np.where(years > 0.0, cumulative, 0.0)


def credit_curve_terms(curve, dates):
    """Cumulative default probability and loss given default, one value per date."""
    probability = np.array([curve.get_pd(day) for day in dates], dtype=float)
    loss = np.array([curve.get_lgd(day) for day in dates], dtype=float)
    return probability, loss


def valuation_adjustment(exposure, discount_factors, default_probability, loss_given_default):
    """Discounted default-weighted exposure.

    ``exposure`` is one profile ``[steps]`` or a panel ``[paths, steps]``. The sum
    starts on the second date::

        DF(t_i) * max(PD(t_i) - PD(t_{i-1}), 0) * exposure(t_i) * LGD(t_i)

    Pass the positive part of exposure for a credit valuation adjustment and the
    negative part for a debit valuation adjustment. A profile returns a float; a
    panel returns one value per path.
    """
    exposure = np.asarray(exposure, dtype=float)
    discount = np.asarray(discount_factors, dtype=float)
    probability = np.asarray(default_probability, dtype=float)
    loss = np.asarray(loss_given_default, dtype=float)
    if exposure.ndim == 1:
        exposure = exposure.reshape(1, -1)
        scalar = True
    elif exposure.ndim == 2:
        scalar = False
    else:
        raise ValueError("exposure must be shaped [steps] or [paths, steps]")
    steps = exposure.shape[1]
    _check_steps(discount, steps, "discount_factors")
    _check_steps(probability, steps, "default_probability")
    _check_steps(loss, steps, "loss_given_default")
    if steps < 2:
        adjustment = np.zeros(exposure.shape[0])
    else:
        interval = np.maximum(np.diff(probability), 0.0)
        adjustment = (discount[1:] * interval * exposure[:, 1:] * loss[1:]).sum(axis=1)
    if scalar:
        return float(adjustment[0])
    return adjustment


def credit_valuation_adjustment(exposure, discount_factors, default_probability, loss_given_default):
    """Valuation adjustment on the positive part of ``exposure``."""
    return valuation_adjustment(
        np.maximum(np.asarray(exposure, dtype=float), 0.0),
        discount_factors,
        default_probability,
        loss_given_default,
    )


def debit_valuation_adjustment(exposure, discount_factors, default_probability, loss_given_default):
    """Valuation adjustment on the negative part of ``exposure``. The result is zero or below."""
    return valuation_adjustment(
        np.minimum(np.asarray(exposure, dtype=float), 0.0),
        discount_factors,
        default_probability,
        loss_given_default,
    )


def cost_of_funding(position, discount_factors, spread_discount_factors):
    """Present cost of funding ``position`` between the risk-free and spread curves.

    ``position`` is ``[steps]`` or ``[paths, steps]``. Both discount-factor arrays
    have one value per step, on the same dates as ``position``. For an expected
    cost, pass the mean position. The first step contributes
    ``(DF - spread DF) * position``. Each later step contributes the previous
    risk-free discount times the change in the spread over the risk-free curve,
    times the position.
    """
    position = np.asarray(position, dtype=float)
    zero = np.asarray(discount_factors, dtype=float)
    spread = np.asarray(spread_discount_factors, dtype=float)
    if position.ndim == 1:
        position = position.reshape(1, -1)
        scalar = True
    elif position.ndim == 2:
        scalar = False
    else:
        raise ValueError("position must be shaped [steps] or [paths, steps]")
    steps = position.shape[1]
    _check_steps(zero, steps, "discount_factors")
    _check_steps(spread, steps, "spread_discount_factors")
    risk_free = zero[0]
    funding = spread[0]
    cost = (risk_free - funding) * position[:, 0]
    for step in range(1, steps):
        next_risk_free = zero[step]
        next_funding = spread[step]
        cost = cost + risk_free * (next_risk_free / risk_free - next_funding / funding) * position[:, step]
        risk_free = next_risk_free
        funding = next_funding
    if scalar:
        return float(cost[0])
    return cost


def regulatory_capital(simulation, netting_sets, factors, credit):
    """Basel II current-exposure capital, shaped ``[paths, exposure dates]``.

    ``netting_sets`` comes from ``PortfolioB2CEM.netting_sets()``. Each set's
    mark-to-market is ``simulation.by_structure``, the value of that set's own
    contracts, so the simulation must have been run with ``by_structure=True``.
    The add-on is spot times the set's effective volume times
    ``factors.get_factor(time)``, scaled by the net-to-gross ratio. Posted
    margin reduces the exposure. ``credit`` supplies the capital charge and
    the counterparty weight.
    """
    if simulation.by_structure is None:
        raise ValueError("regulatory capital needs a simulation with by_structure=True")
    set_values = np.asarray(simulation.by_structure, dtype=float)
    spot = np.asarray(simulation.spot, dtype=float)
    dates = list(simulation.dates)
    paths, steps, sets = set_values.shape
    described = list(netting_sets)
    if sets != len(described):
        raise ValueError("netting_sets must match the simulated structures")
    if not described:
        return np.zeros((paths, len(simulation.exposure_steps)))

    net = set_values.sum(axis=2)
    gross = np.maximum(set_values, 0.0).sum(axis=2)
    ratio = np.zeros_like(net)
    usable = (net >= 0.0) & (gross > 1e-14)
    ratio[usable] = net[usable] / gross[usable]

    total = np.zeros((paths, steps))
    for index, netting in enumerate(described):
        settlement = _field(netting, "settlement")
        alive = np.array([day <= settlement for day in dates])
        if not _field(netting, "has_price_exposure"):
            piece = np.where(alive, float(_field(netting, "cap")), 0.0)
        else:
            years = np.array([max((settlement - day).days, 0) / 365.0 for day in dates])
            addon_factor = np.array([factors.get_factor(float(year)) for year in years])
            addon = spot * float(_field(netting, "volume")) * addon_factor * (0.4 + 0.6 * ratio)
            piece = set_values[:, :, index] + addon
            if _field(netting, "capped"):
                piece = np.minimum(piece, float(_field(netting, "cap")))
            piece = np.where(alive, piece, 0.0)
        total += piece

    if simulation.margin is None:
        capital = total - net + np.maximum(net, 0.0)
    else:
        margin = np.asarray(simulation.margin, dtype=float)
        capital = total - net + np.maximum(net - margin, 0.0)
    scale = float(credit.capital_charge()) * float(credit.counterparty_weighting())
    capital = np.maximum(capital * scale, 0.0)
    return capital[:, list(simulation.exposure_steps)]


def _field(netting, name):
    if isinstance(netting, dict):
        return netting[name]
    return getattr(netting, name)


def _check_steps(values, steps, name):
    if values.shape != (steps,):
        raise ValueError(f"{name} must have one value per step")


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
