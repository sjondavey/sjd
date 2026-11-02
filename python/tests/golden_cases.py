"""Golden values: the numbers the library produced at the pre-modernisation baseline.

``compute()`` rebuilds every case from scratch. ``test_golden.py`` compares it
with ``golden/golden.json``. After an intended change to the numbers, regenerate
the file and explain the change in the commit message::

    python python/tests/golden_cases.py
"""

from __future__ import annotations

import json
import sys
from datetime import date, timedelta
from pathlib import Path

import numpy as np

import sjd
from sjd import examples, stats

GOLDEN_PATH = Path(__file__).with_name("golden") / "golden.json"

SEED = 42
PATHS = 2000
FIRST_PATHS = 3


def _ok(source, name):
    if not source.is_ok():
        raise RuntimeError(f"golden case {name} is not valid: {source.error_messages()}")


def _black76():
    out = {}
    grid = [
        (forward, strike, sd, df)
        for forward in (80.0, 100.0, 120.0)
        for strike in (90.0, 100.0, 110.0)
        for sd in (0.05, 0.2, 0.5)
        for df in (1.0, 0.95)
    ]
    for kind, cls in (("call", sjd.Black76Call), ("put", sjd.Black76Put)):
        premiums, deltas = [], []
        for forward, strike, sd, df in grid:
            option = cls(forward, strike, sd, df)
            if not option.is_ok():
                raise RuntimeError(option.error_message())
            premiums.append(option.premium())
            deltas.append(option.delta())
        out[f"black76.{kind}.premium"] = premiums
        out[f"black76.{kind}.delta"] = deltas
    return out


def _lookup_dates(market):
    curve_dates = market["curve_dates"]
    days = list(curve_dates)
    for first, second in zip(curve_dates, curve_dates[1:]):
        days.append(first + (second - first) / 2)
    return sorted(set(days))


def _curves(market):
    days = _lookup_dates(market)
    return {
        "curve.lookup_dates": [day.isoformat() for day in days],
        "curve.forward": [market["forward"].get_forward(day) for day in days],
        "curve.discount": [market["discount"].get_discount_factor(day) for day in days],
        "curve.volatility": [market["volatility"].get_volatility(day) for day in days],
    }


def _surfaces(market):
    anchor = market["anchor"]
    out = {}

    cma_dates = [date(2026, month, 1) for month in range(10, 13)] + [
        date(2027, month, 1) for month in range(1, 13)
    ]
    cma_vols = [0.20 - 0.004 * i for i in range(len(cma_dates))]
    cma_curve = sjd.VolatilityCurveCMA(anchor, cma_dates, cma_vols)
    _ok(cma_curve, "VolatilityCurveCMA")
    cma_lookups = [date(2026, 11, 15), date(2027, 2, 10), date(2027, 6, 20), date(2027, 12, 1)]
    out["surface.cma_curve.volatility"] = [cma_curve.get_volatility(day) for day in cma_lookups]
    out["surface.cma_curve.standard_deviation"] = [
        cma_curve.get_standard_deviation(day) for day in cma_lookups
    ]

    surface_dates = [anchor + timedelta(days=days) for days in (30, 91, 182, 365, 730)]
    deltas = [10.0, 25.0, 50.0, 75.0, 90.0]
    delta_vols = [
        [0.16 + 0.01 * row - 0.002 * column for column in range(len(surface_dates))]
        for row in range(len(deltas))
    ]
    moneyness = [0.8, 0.9, 1.0, 1.1, 1.2]
    moneyness_vols = [
        [0.18 + 0.02 * abs(row - 2) + 0.001 * column for column in range(len(cma_dates))]
        for row in range(len(moneyness))
    ]
    lookups = [
        (anchor + timedelta(days=days), strike, 17.5) for days in (45, 200, 500) for strike in (15.0, 17.5, 20.0)
    ]
    for name, interpolator in (
        ("bilinear", sjd.SurfaceInterpolator.BILINEAR),
        ("bicubic", sjd.SurfaceInterpolator.BICUBIC),
    ):
        surface = sjd.VolatilitySurfaceDelta(anchor, surface_dates, deltas, delta_vols, interpolator)
        _ok(surface, f"VolatilitySurfaceDelta {name}")
        out[f"surface.delta.{name}"] = [surface.get_volatility(*lookup) for lookup in lookups]
        surface = sjd.VolatilitySurfaceMoneynessCMA(anchor, cma_dates, moneyness, moneyness_vols, interpolator)
        _ok(surface, f"VolatilitySurfaceMoneynessCMA {name}")
        out[f"surface.moneyness_cma.{name}"] = [surface.get_volatility(*lookup) for lookup in lookups]
    return out


def _contracts(market):
    environment = market["environment"]
    forward = market["forward"]
    book = examples.sample_contracts(forward, market)
    out = {}
    for name in ("put", "call", "forward", "asian"):
        contract = book[name]
        contract.validate(environment)
        _ok(contract, name)
        out[f"contract.sample.{name}"] = contract.get_value(environment)

    settlement = market["collar_settlement"]
    maturity = market["collar_maturity"]
    values = []
    for strike in (15.0, 17.0, 17.75, 19.0):
        for kind in (sjd.PutCall.PUT, sjd.PutCall.CALL):
            for buy_sell in (sjd.BuySell.BUY, sjd.BuySell.SELL):
                option = sjd.EuropeanOption(
                    maturity=maturity,
                    settlement=settlement,
                    strike=strike,
                    type=kind,
                    volume=1000.0,
                    premium_payment_date=settlement,
                    premium_amount=0.25,
                    buy_sell=buy_sell,
                )
                option.validate(environment)
                _ok(option, "European option")
                values.append(option.get_value(environment))
                values.append(option.get_option_value(environment))
    out["contract.european_option"] = values

    cash_settled = sjd.EuropeanForward(
        maturity=maturity, settlement=settlement, strike=17.6, volume=1000.0, buy_sell=sjd.BuySell.SELL
    )
    cash_settled.validate(environment)
    _ok(cash_settled, "cash-settled forward")
    out["contract.cash_settled_forward"] = cash_settled.get_value(environment)

    # One averaging date before the anchor, so a fixing is read.
    averaging = [date(2026, 9, 15)] + market["averaging_dates"][:10]
    asian_settlement = market["averaging_dates"][12]
    asian_values = []
    for kind in (sjd.PutCall.PUT, sjd.PutCall.CALL):
        for strike in (17.0, 18.5, 20.0):
            option = sjd.AsianOption(
                averaging_dates=averaging,
                settlement=asian_settlement,
                strike=strike,
                type=kind,
                volume=1000.0,
                premium_payment_date=asian_settlement,
                premium_amount=0.1,
                buy_sell=sjd.BuySell.BUY,
            )
            option.validate(environment)
            _ok(option, "Asian option")
            asian_values.append(option.get_value(environment))
            asian_values.append(option.get_option_value(environment))
    out["contract.asian_option"] = asian_values

    payoffs = []
    for spot in (10.0, 17.0, 18.0, 25.0):
        payoffs.extend(book[name].get_payoff(spot) for name in ("put", "call", "forward", "asian"))
    out["contract.sample.payoff"] = payoffs
    return out


def _capital(market):
    environment = market["environment"]
    book = examples.sample_contracts(market["forward"], market)
    factors = sjd.reg_cap_factors("fx")
    credit = examples.credit_parameters()
    out = {}
    portfolio = sjd.Portfolio(book["contracts"])
    portfolio.validate(environment)
    out["portfolio.value"] = portfolio.get_value(environment)
    for optimise in (False, True):
        cem = sjd.PortfolioB2CEM(book["contracts"], factors, credit, optimise)
        _ok(cem, f"PortfolioB2CEM optimise={optimise}")
        key = f"capital.cem.optimise_{str(optimise).lower()}"
        out[key] = cem.get_regulatory_capital(environment)
        sets = cem.netting_sets()
        out[key + ".netting_sets"] = [
            [entry["volume"], float(entry["capped"]), float(entry["has_price_exposure"]), entry["cap"]]
            for entry in sets
        ]
    margin = sjd.SimpleMargin(market["anchor"], True, 50_000.0, True, 50_000.0, 10_000.0, 1_000.0)
    margined = sjd.PortfolioB2CEM(book["contracts"], factors, credit, True, margin)
    margined.set_margin_balance(100_000.0)
    out["capital.cem.margined"] = margined.get_regulatory_capital(environment)
    out["margin.call"] = [margin.get_margin_call(mtm) for mtm in (-250_000.0, -40_000.0, 0.0, 75_000.0, 1e6)]
    return out


def _credit(market):
    anchor = market["anchor"]
    days = [anchor + timedelta(days=days) for days in (0, 30, 180, 365, 1000)]
    flat = sjd.FlatDefaultProbability(anchor, 0.02, 0.6)
    cumulative = sjd.CumulativeDefaultProbability(
        anchor,
        [anchor + timedelta(days=365 * years) for years in (1, 2, 5)],
        [0.01, 0.025, 0.08],
        [0.6, 0.55, 0.5],
    )
    return {
        "credit.flat.pd": [flat.get_pd(day) for day in days],
        "credit.cumulative.pd": [cumulative.get_pd(day) for day in days[:-1]],
        "credit.cumulative.lgd": [cumulative.get_lgd(day) for day in days[:-1]],
    }


def _simulation():
    market = examples.sample_market(stochastic=True)
    environment = market["environment"]
    dates = market["simulation_dates"]
    book = examples.sample_contracts(market["forward"], market)
    factors = sjd.reg_cap_factors("fx")
    credit = examples.credit_parameters()
    out = {}

    def record(prefix, simulation):
        values = stats.exposure(simulation)
        out[prefix + ".mean_spot"] = np.asarray(simulation.spot).mean(axis=0).tolist()
        out[prefix + ".mean_mtm"] = np.asarray(simulation.mtm).mean(axis=0).tolist()
        out[prefix + ".ee"] = stats.expected_exposure(values).tolist()
        out[prefix + ".pfe"] = stats.potential_future_exposure(values, 0.95).tolist()
        out[prefix + ".first_spot"] = np.asarray(simulation.spot)[:FIRST_PATHS].ravel().tolist()
        out[prefix + ".first_mtm"] = np.asarray(simulation.mtm)[:FIRST_PATHS].ravel().tolist()
        if simulation.margin is not None:
            out[prefix + ".mean_margin"] = np.asarray(simulation.margin).mean(axis=0).tolist()

    unmargined = sjd.PortfolioB2CEM(book["contracts"], factors, credit, True)
    simulation = sjd.simulate(unmargined, environment, dates, paths=PATHS, seed=SEED, by_structure=True)
    record("simulation.unmargined", simulation)
    capital = stats.regulatory_capital(simulation, unmargined.netting_sets(), factors, credit)
    out["simulation.unmargined.mean_capital"] = capital.mean(axis=0).tolist()

    margin = sjd.SimpleMargin(market["anchor"], True, 50_000.0, True, 50_000.0, 10_000.0, 1_000.0)
    margined = sjd.Portfolio(book["contracts"], margin)
    simulation = sjd.simulate(
        margined,
        environment,
        dates,
        paths=PATHS,
        seed=SEED,
        holding_period=10,
        holding_period_calendar=market["calendar"],
    )
    record("simulation.margined", simulation)
    out["simulation.margined.dates"] = [day.isoformat() for day in simulation.dates]
    out["simulation.margined.hp_ee"] = stats.expected_exposure(stats.holding_period_exposure(simulation)).tolist()
    return out


def compute() -> dict:
    market = examples.sample_market()
    results = {}
    for part in (_black76(), _curves(market), _surfaces(market), _contracts(market),
                 _capital(market), _credit(market), _simulation()):
        overlap = results.keys() & part.keys()
        if overlap:
            raise RuntimeError(f"duplicate golden keys: {sorted(overlap)}")
        results.update(part)
    return results


def main() -> int:
    results = compute()
    GOLDEN_PATH.parent.mkdir(exist_ok=True)
    payload = {
        "_comment": "Generated by python/tests/golden_cases.py. Regenerate only for an intended change.",
        "values": results,
    }
    GOLDEN_PATH.write_text(json.dumps(payload, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    print(f"wrote {len(results)} golden entries to {GOLDEN_PATH}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
