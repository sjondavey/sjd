"""Shared setup for the sjd teaching notebooks.

Notebooks live in this directory. Jupyter puts that directory on ``sys.path``,
so ``import sjd_setup`` works from a notebook kernel. ``load_sjd`` then adds
``python/sjd_runtime`` and imports the extension module.
"""

import math
import sys
from calendar import monthrange
from datetime import date
from pathlib import Path

ANCHOR = date(2026, 9, 16)
SPOT = 17.50
DOMESTIC_RATE = 0.07
FOREIGN_RATE = 0.04
FLAT_VOLATILITY = 0.12


def repo_root():
    """Return the sjd repository root that contains CMakeLists.txt."""
    candidate = Path(__file__).resolve().parents[2]
    if (candidate / "CMakeLists.txt").is_file() and (candidate / "python").is_dir():
        return candidate
    cwd = Path.cwd().resolve()
    for folder in [cwd, *cwd.parents]:
        if (folder / "CMakeLists.txt").is_file() and (folder / "python").is_dir():
            return folder
    raise RuntimeError("Could not find the sjd repo root from " + str(cwd))


def load_sjd():
    """Import the staged ``sjd`` extension from ``python/sjd_runtime``."""
    runtime = str(repo_root() / "python" / "sjd_runtime")
    if runtime not in sys.path:
        sys.path.insert(0, runtime)
    import sjd

    return sjd


def year_fraction(day, anchor=ANCHOR):
    """Actual/365 year fraction from ``anchor`` to ``day``."""
    return (day - anchor).days / 365.0


def fx_curve_values(
    curve_dates,
    anchor=ANCHOR,
    spot=SPOT,
    domestic_rate=DOMESTIC_RATE,
    foreign_rate=FOREIGN_RATE,
    flat_volatility=FLAT_VOLATILITY,
):
    """Forwards, discount factors, and flat volatilities for an FX-style strip.

    Forwards use the interest-rate differential. Discount factors use the
    domestic continuously compounded rate.
    """
    forwards = [
        spot * math.exp((domestic_rate - foreign_rate) * year_fraction(day, anchor))
        for day in curve_dates
    ]
    discount_factors = [
        math.exp(-domestic_rate * year_fraction(day, anchor)) for day in curve_dates
    ]
    volatilities = [flat_volatility] * len(curve_dates)
    return forwards, discount_factors, volatilities


def fx_curves(sjd, curve_dates, allow_extrapolation=False, **kwargs):
    """Build deterministic forward, discount, and volatility curves."""
    anchor = kwargs.get("anchor", ANCHOR)
    forwards, discount_factors, volatilities = fx_curve_values(curve_dates, **kwargs)
    forward = sjd.ForwardCurve(
        anchor, curve_dates, forwards, allow_extrapolation=allow_extrapolation
    )
    discount = sjd.DiscountCurve(
        anchor, curve_dates, discount_factors, allow_extrapolation=allow_extrapolation
    )
    volatility = sjd.VolatilityCurve(
        anchor, curve_dates, volatilities, allow_extrapolation=allow_extrapolation
    )
    return forward, discount, volatility


def add_months(day, months):
    month_index = day.month - 1 + months
    year = day.year + month_index // 12
    month = month_index % 12 + 1
    return date(year, month, min(day.day, monthrange(year, month)[1]))


def sample_calendar(sjd):
    """USDZAR holidays: closed when either the United States or South Africa is.

    ``calendar.adjust(day)`` rolls to the next business day and
    ``calendar.advance(day, n)`` moves ``n`` business days.
    """
    return sjd.Calendar("USDZAR", ["UnitedStates", "SouthAfrica"])


def month_start(year, month):
    return date(year, month, 1)


def business_days_in_month(calendar, year, month):
    last_day = monthrange(year, month)[1]
    return [
        date(year, month, day)
        for day in range(1, last_day + 1)
        if calendar.is_business_day(date(year, month, day))
    ]


def sample_market(sjd, stochastic=False):
    """The commodity book market used by the portfolio notebooks.

    One collar month, one forward settlement, and one Asian averaging month
    sit on a single FX-style curve. ``stochastic=True`` gives the forward a
    ``ForwardRoll.FORWARD_VOLATILITY`` roll so a ``PortfolioEvolver`` can roll it.

    Simulation dates stop at the forward settlement, before the Asian averaging
    month. A rolled fixing curve only receives the simulation dates, so the
    Asian can be valued on the path while every averaging day is still a forward.
    """
    anchor = ANCHOR
    holidays = sample_calendar(sjd)
    collar_settlement = holidays.adjust(add_months(anchor, 3))
    collar_maturity = holidays.advance(collar_settlement, -2)
    forward_settlement = holidays.adjust(add_months(anchor, 6))
    averaging_dates = business_days_in_month(holidays, 2027, 6)
    asian_settlement = holidays.advance(averaging_dates[-1], 2)
    # A roll onto the last simulation date still needs a pillar beyond that date.
    curve_end = add_months(asian_settlement, 12)
    curve_dates = sorted(
        {
            anchor,
            collar_maturity,
            collar_settlement,
            forward_settlement,
            month_start(2027, 6),
            asian_settlement,
            curve_end,
        }
    )
    forwards, discount_factors, volatilities = fx_curve_values(curve_dates)
    discount = sjd.DiscountCurve(
        anchor, curve_dates, discount_factors, allow_extrapolation=False
    )
    volatility = sjd.VolatilityCurve(
        anchor, curve_dates, volatilities, allow_extrapolation=False
    )
    roll = sjd.ForwardRoll.FORWARD_VOLATILITY if stochastic else sjd.ForwardRoll.DETERMINISTIC
    forward = sjd.ForwardCurve(
        anchor,
        curve_dates,
        forwards,
        allow_extrapolation=False,
        roll=roll,
        volatility=volatility,
    )
    fixings = sjd.DailyFixingCurve([date(2026, 9, 15)], [SPOT])
    environment = sjd.RatesEnvironment(anchor)
    environment.set_frs(forward)
    environment.set_drs(discount)
    environment.set_vrs(volatility)
    environment.set_fixing_rs(fixings)
    return {
        "anchor": anchor,
        "calendar": holidays,
        "curve_dates": curve_dates,
        "forwards": forwards,
        "environment": environment,
        "forward": forward,
        "discount": discount,
        "volatility": volatility,
        "fixings": fixings,
        "collar_maturity": collar_maturity,
        "collar_settlement": collar_settlement,
        "forward_settlement": forward_settlement,
        "averaging_dates": averaging_dates,
        "asian_settlement": asian_settlement,
        "simulation_dates": [
            anchor,
            collar_settlement,
            forward_settlement,
        ],
    }


def sample_contracts(sjd, forward_curve, market):
    """Collar, outright forward, and Asian forward on ``sample_market`` dates.

    The forward and the Asian are struck below the curve so the book has
    positive mark-to-market. That is the case where posted variation margin
    reduces Basel II current-exposure capital.
    """
    volume = 1_000_000.0
    atm = forward_curve.get_forward(market["collar_settlement"])
    put_strike = round(atm * 0.97, 2)
    call_strike = round(atm * 1.03, 2)
    put = sjd.EuropeanOption(
        maturity=market["collar_maturity"],
        settlement=market["collar_settlement"],
        strike=put_strike,
        type=sjd.PutCall.PUT,
        volume=volume,
        premium_payment_date=market["collar_settlement"],
        premium_amount=0.0,
        buy_sell=sjd.BuySell.BUY,
    )
    call = sjd.EuropeanOption(
        maturity=market["collar_maturity"],
        settlement=market["collar_settlement"],
        strike=call_strike,
        type=sjd.PutCall.CALL,
        volume=volume,
        premium_payment_date=market["collar_settlement"],
        premium_amount=0.0,
        buy_sell=sjd.BuySell.SELL,
    )
    forward_strike = round(forward_curve.get_forward(market["forward_settlement"]) * 0.98, 2)
    outright = sjd.EuropeanForward(
        settlement=market["forward_settlement"],
        strike=forward_strike,
        volume=volume,
        buy_sell=sjd.BuySell.BUY,
    )
    average_forward = sum(
        forward_curve.get_forward(day) for day in market["averaging_dates"]
    ) / len(market["averaging_dates"])
    asian_strike = round(average_forward * 0.99, 2)
    asian = sjd.AsianForward(
        averaging_dates=market["averaging_dates"],
        settlement=market["asian_settlement"],
        strike=asian_strike,
        volume=500_000.0,
        buy_sell=sjd.BuySell.BUY,
    )
    return {
        "contracts": [put, call, outright, asian],
        "put": put,
        "call": call,
        "forward": outright,
        "asian": asian,
        "put_strike": put_strike,
        "call_strike": call_strike,
        "forward_strike": forward_strike,
        "asian_strike": asian_strike,
        "atm": atm,
        "volume": volume,
    }


def credit_parameters(sjd, counterparty_weighting=1.0, capital_charge=0.08):
    """Basel-style scalers: exposure times weight times capital charge."""
    return sjd.RegCapCreditParameters(counterparty_weighting, capital_charge)
