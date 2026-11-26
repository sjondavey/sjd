"""Revalue strategies on fixed spot paths and search cheap parameters.

Draw spots once with ``draw_spot_paths``, then ``value_on_spots`` marks each
candidate book to those same paths so Monte Carlo noise is common across
trials. ``minimise_collar_put_moneyness`` is a one-dimensional grid search
over the put wing of a collar.
"""

from __future__ import annotations

import logging
from collections.abc import Sequence

import numpy as np

import sjd
from sjd import stats
from sjd.strategy import CollarStrategy

logger = logging.getLogger(__name__)


def draw_spot_paths(
    portfolio,
    rates_environment,
    simulation_dates,
    paths=256,
    seed=None,
    antithetic=True,
    holding_period=0,
    holding_period_calendar=None,
):
    """Simulate once and return the spot array ``[paths, steps]``."""
    simulation = sjd.simulate(
        portfolio,
        rates_environment,
        simulation_dates,
        paths=paths,
        seed=seed,
        antithetic=antithetic,
        holding_period=holding_period,
        holding_period_calendar=holding_period_calendar,
    )
    return np.asarray(simulation.spot, dtype=float)


def make_evolver(
    portfolio,
    rates_environment,
    simulation_dates,
    holding_period=0,
    holding_period_calendar=None,
):
    """Portfolio evolver on ``rates_environment`` for the given dates."""
    generator = sjd.SingleFactorEconomicScenarioGenerator(rates_environment)
    return sjd.PortfolioEvolver(
        portfolio,
        generator,
        simulation_dates,
        holding_period=holding_period,
        holding_period_calendar=holding_period_calendar,
    )


def value_on_spots(
    portfolio,
    rates_environment,
    simulation_dates,
    spots,
    *,
    by_trade=False,
    by_structure=False,
    holding_period=0,
    holding_period_calendar=None,
):
    """Mark ``portfolio`` to every row of ``spots`` without redrawing shocks."""
    spots = np.asarray(spots, dtype=float)
    if spots.ndim != 2:
        raise ValueError("spots must be shaped [paths, steps]")
    evolver = make_evolver(
        portfolio,
        rates_environment,
        simulation_dates,
        holding_period=holding_period,
        holding_period_calendar=holding_period_calendar,
    )
    sjd.require_valid(evolver)
    return evolver.value_spot_paths(spots, by_trade=by_trade, by_structure=by_structure)


def evaluate_strategy_cost(
    portfolio,
    rates_environment,
    simulation_dates,
    spots,
    discount_factors,
    default_probability,
    loss_given_default,
    *,
    by_structure=False,
    netting_sets=None,
    factors=None,
    credit=None,
    capital_cost_rate=0.0,
    funding_spread_discount_factors=None,
    funding_position=None,
    holding_period=0,
    holding_period_calendar=None,
):
    """Value on fixed spots and return ``stats.structure_costs``."""
    simulation = value_on_spots(
        portfolio,
        rates_environment,
        simulation_dates,
        spots,
        by_structure=by_structure,
        holding_period=holding_period,
        holding_period_calendar=holding_period_calendar,
    )
    return stats.structure_costs(
        simulation,
        discount_factors,
        default_probability,
        loss_given_default,
        netting_sets=netting_sets,
        factors=factors,
        credit=credit,
        capital_cost_rate=capital_cost_rate,
        funding_spread_discount_factors=funding_spread_discount_factors,
        funding_position=funding_position,
    )


def minimise_collar_put_moneyness(
    market,
    forward_curve,
    rates_environment,
    simulation_dates,
    spots,
    put_moneyness: Sequence[float],
    call_moneyness: float,
    volume: float,
    discount_factors,
    default_probability,
    loss_given_default,
    *,
    objective: str = "total",
    premium_weight: float = 0.0,
    factors=None,
    credit=None,
    capital_cost_rate=0.0,
    funding_spread_discount_factors=None,
    margin=None,
    use_b2cem: bool = False,
) -> dict:
    """Grid-search put moneyness for a collar on fixed spot paths.

    Each trial builds a ``CollarStrategy.from_moneyness``, values it on
    ``spots``, and scores ``structure_costs[objective]`` plus
    ``premium_weight * net option premium`` (the bank's option value today,
    with sold options negative, not path-averaged). When ``use_b2cem`` is true, capital uses CEM netting sets
    from a ``Portfolio`` and a ``CurrentExposureMethod`` and needs ``factors`` and ``credit``.

    Returns the best ``strategy``, ``costs``, ``premium``, ``score``, and the
    full ``trials`` list.
    """
    if objective not in {"total", "cva", "dva", "funding", "capital_cost", "capital"}:
        raise ValueError(f"unknown objective {objective!r}")
    trials = []
    best = None
    for put_m in put_moneyness:
        strategy = CollarStrategy.from_moneyness(
            maturity=market["collar_maturity"],
            settlement=market["collar_settlement"],
            forward=forward_curve.get_forward(market["collar_settlement"]),
            put_moneyness=float(put_m),
            call_moneyness=call_moneyness,
            volume=volume,
        )
        if use_b2cem:
            if factors is None or credit is None:
                raise ValueError("use_b2cem needs factors and credit")
            portfolio = strategy.portfolio_b2cem(factors, credit, True, margin)
            netting = portfolio.netting_sets()
            costs = evaluate_strategy_cost(
                portfolio,
                rates_environment,
                simulation_dates,
                spots,
                discount_factors,
                default_probability,
                loss_given_default,
                by_structure=True,
                netting_sets=netting,
                factors=factors,
                credit=credit,
                capital_cost_rate=capital_cost_rate,
                funding_spread_discount_factors=funding_spread_discount_factors,
            )
        else:
            portfolio = strategy.portfolio(margin)
            costs = evaluate_strategy_cost(
                portfolio,
                rates_environment,
                simulation_dates,
                spots,
                discount_factors,
                default_probability,
                loss_given_default,
                funding_spread_discount_factors=funding_spread_discount_factors,
            )
        premium = strategy.premium(rates_environment)
        score = float(costs[objective]) + float(premium_weight) * float(premium)
        logger.info("optimiser trial put_moneyness=%s score=%s", float(put_m), score)
        trial = {
            "put_moneyness": float(put_m),
            "strategy": strategy,
            "costs": costs,
            "premium": premium,
            "score": score,
        }
        trials.append(trial)
        if best is None or score < best["score"]:
            best = trial
    assert best is not None
    return {
        "strategy": best["strategy"],
        "costs": best["costs"],
        "premium": best["premium"],
        "score": best["score"],
        "trials": trials,
    }


_OBJECTIVES = {"total", "cva", "dva", "funding", "capital_cost", "capital"}


def minimise_strategy(
    build,
    x0,
    rates_environment,
    simulation_dates,
    spots,
    discount_factors,
    default_probability,
    loss_given_default,
    *,
    bounds=None,
    objective: str = "total",
    premium_weight: float = 0.0,
    factors=None,
    credit=None,
    capital_cost_rate=0.0,
    funding_spread_discount_factors=None,
    margin=None,
    use_b2cem: bool = False,
    method: str = "L-BFGS-B",
    options=None,
) -> dict:
    """Search strategy parameters with ``scipy.optimize`` on fixed spot paths.

    ``build(parameters)`` returns a strategy. Every trial is marked to
    ``spots``, so the Monte Carlo noise is common across the search. The score
    is ``structure_costs[objective]`` plus ``premium_weight * premium``.
    ``method`` defaults to ``L-BFGS-B``, which honours ``bounds``.
    """
    try:
        from scipy.optimize import minimize
    except ImportError as error:
        raise ImportError(
            "scipy is required for minimise_strategy; install the sjd front extra"
        ) from error
    if objective not in _OBJECTIVES:
        raise ValueError(f"unknown objective {objective!r}")

    def score_of(parameters):
        strategy = build(np.asarray(parameters, dtype=float))
        if use_b2cem:
            if factors is None or credit is None:
                raise ValueError("use_b2cem needs factors and credit")
            portfolio = strategy.portfolio_b2cem(factors, credit, True, margin)
            costs = evaluate_strategy_cost(
                portfolio,
                rates_environment,
                simulation_dates,
                spots,
                discount_factors,
                default_probability,
                loss_given_default,
                by_structure=True,
                netting_sets=portfolio.netting_sets(),
                factors=factors,
                credit=credit,
                capital_cost_rate=capital_cost_rate,
                funding_spread_discount_factors=funding_spread_discount_factors,
            )
        else:
            portfolio = strategy.portfolio(margin)
            costs = evaluate_strategy_cost(
                portfolio,
                rates_environment,
                simulation_dates,
                spots,
                discount_factors,
                default_probability,
                loss_given_default,
                funding_spread_discount_factors=funding_spread_discount_factors,
            )
        premium = strategy.premium(rates_environment)
        score = float(costs[objective]) + float(premium_weight) * float(premium)
        logger.info("optimiser parameters=%s score=%s", np.asarray(parameters), score)
        return score, strategy, costs, premium

    searched = minimize(
        lambda parameters: score_of(parameters)[0],
        np.asarray(x0, dtype=float),
        method=method,
        bounds=bounds,
        options=options,
    )
    score, strategy, costs, premium = score_of(searched.x)
    return {
        "strategy": strategy,
        "parameters": np.asarray(searched.x, dtype=float),
        "costs": costs,
        "premium": premium,
        "score": score,
        "result": searched,
    }


def solve_pv(build, market, target_pv=0.0, bracket=(0.01, 10_000.0), xtol=1e-6):
    """Find one parameter that makes a structure worth ``target_pv``.

    This is the deterministic search. ``minimise_strategy`` is the search on
    simulated paths. See ``sjd.structuring.solve_pv``.
    """
    from sjd.structuring import solve_pv as _solve_pv

    return _solve_pv(build, market, target_pv=target_pv, bracket=bracket, xtol=xtol)
