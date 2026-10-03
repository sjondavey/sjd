"""Build a RatesEnvironment from stored pillars and the market spec."""

import calendar as calendar_module
import functools
import math
import sqlite3
from datetime import date, timedelta

from typing import Optional

from .specs import (
    CATALOGUE,
    CalendarMonthDates,
    CalendarMonthMoneynessVol,
    CalendarSpec,
    Catalogue,
    CurveInterpolation,
    DeltaVol,
    ForwardAveraging,
    ForwardRoll,
    GofoForward,
    MetalTimesUsdZarForward,
    OutrightForward,
    PointDates,
    QuoteNotAvailable,
    QuoteSpec,
    SurfaceInterpolation,
    require_roll,
)
from .store import (
    discount_pillars,
    forward_pillars,
    spot,
    volatility_grid,
)


def rates_environment(
    market: str,
    anchor: date,
    currency: str,
    connection: sqlite3.Connection,
    roll: Optional[ForwardRoll] = None,
    catalogue: Optional[Catalogue] = None,
):
    """Forward, discount, and volatility for one quote, ready to price.

    ``roll`` replaces the quote's default ForwardRoll. A DETERMINISTIC
    environment prices but cannot drive a Monte Carlo. FORWARD_VOLATILITY and
    SPOT_VOLATILITY simulate the forward with the quote's own surface, which
    is also the environment's volatility.

    ``catalogue`` defaults to ``catalogue.toml``.
    """
    sjd = _sjd()
    catalogue = catalogue or CATALOGUE
    spec = catalogue.quote_spec(market, currency)
    for calendar_spec in _calendar_specs(spec):
        calendar(calendar_spec)
    if roll is None:
        roll = spec.forward.roll
    require_roll(spec.forward.averaging, roll, f"{spec.market} {spec.currency} forward")
    discount = _discount_curve(sjd, anchor, spec.discount_currency, spec.discount, connection)
    volatility = _volatility_surface(sjd, anchor, spec, connection)
    forward = _forward_curve(sjd, anchor, spec, connection, roll, volatility, catalogue)
    environment = sjd.RatesEnvironment(anchor)
    environment.set_frs(forward)
    environment.set_drs(discount)
    environment.set_vrs(volatility)
    return environment


@functools.lru_cache(maxsize=None)
def calendar(spec: CalendarSpec):
    """The ``sjd.Calendar`` for a spec. Raises ValueError for an unknown QuantLib name."""
    return _sjd().Calendar(spec.name, list(spec.components))


def business_days_after(day: date, count: int, calendar_spec: CalendarSpec) -> date:
    """Move ``count`` business days, skipping weekends and the calendar's holidays."""
    return calendar(calendar_spec).advance(day, count)


def spot_date(spec: QuoteSpec, anchor: date) -> date:
    """The quote's spot date: ``spot_lag_business_days`` after ``anchor``."""
    dates = _point_dates(spec)
    return business_days_after(anchor, dates.spot_lag_business_days, dates.calendar)


def option_expiry(spec: QuoteSpec, value_date: date) -> date:
    """Expiry of an option that settles on ``value_date``."""
    dates = _point_dates(spec)
    return business_days_after(value_date, -dates.expiry_before_value_business_days, dates.calendar)


def pricing_dates(spec: QuoteSpec, day: date) -> list[date]:
    """Publication business days in the contract month that contains ``day``."""
    dates = _month_dates(spec)
    publication = calendar(dates.publication_calendar)
    first = day.replace(day=1)
    last = first.replace(day=calendar_module.monthrange(first.year, first.month)[1])
    return [
        first + timedelta(days=offset)
        for offset in range((last - first).days + 1)
        if publication.is_business_day(first + timedelta(days=offset))
    ]


def month_settlement(spec: QuoteSpec, day: date) -> date:
    """Cash settlement for the contract month: T + ``settlement_lag_business_days``.

    T is the last pricing date of the month. The lag counts business days on
    the settlement calendar.
    """
    dates = _month_dates(spec)
    final_pricing = pricing_dates(spec, day)[-1]
    return business_days_after(final_pricing, dates.settlement_lag_business_days, dates.settlement_calendar)


def _month_dates(spec: QuoteSpec) -> CalendarMonthDates:
    if not isinstance(spec.dates, CalendarMonthDates):
        raise QuoteNotAvailable(f"{spec.market} {spec.currency} dates are not a contract month.")
    return spec.dates


def _point_dates(spec: QuoteSpec) -> PointDates:
    if not isinstance(spec.dates, PointDates):
        raise QuoteNotAvailable(f"{spec.market} {spec.currency} dates are not a spot lag.")
    return spec.dates


def _calendar_specs(spec: QuoteSpec):
    if isinstance(spec.dates, PointDates):
        return (spec.dates.calendar,)
    if isinstance(spec.dates, CalendarMonthDates):
        return (spec.dates.publication_calendar, spec.dates.settlement_calendar)
    raise TypeError(f"No calendars for {type(spec.dates).__name__}.")


def _sjd():
    import sjd

    return sjd


def _require_linear(interpolation: CurveInterpolation, what: str) -> None:
    if interpolation is not CurveInterpolation.LINEAR:
        raise NotImplementedError(
            f"{what} uses {interpolation.value}. "
            "The Python discount curve binding builds a linear curve."
        )


def _curve_interpolator(sjd, interpolation: CurveInterpolation):
    if interpolation is CurveInterpolation.LINEAR:
        return sjd.CurveInterpolator.LINEAR
    if interpolation is CurveInterpolation.CUBIC:
        return sjd.CurveInterpolator.CUBIC
    raise TypeError(f"Unknown curve interpolation {interpolation!r}.")


def _sjd_averaging(sjd, averaging: ForwardAveraging):
    if not isinstance(averaging, ForwardAveraging):
        raise TypeError(f"Unknown forward averaging {averaging!r}.")
    return getattr(sjd.ForwardAveraging, averaging.name)


def _sjd_roll(sjd, roll: ForwardRoll):
    if not isinstance(roll, ForwardRoll):
        raise TypeError(f"Unknown forward roll {roll!r}.")
    return getattr(sjd.ForwardRoll, roll.name)


def _surface_interpolator(sjd, interpolation: SurfaceInterpolation):
    if interpolation is SurfaceInterpolation.BILINEAR:
        return sjd.SurfaceInterpolator.BILINEAR
    if interpolation is SurfaceInterpolation.BICUBIC:
        return sjd.SurfaceInterpolator.BICUBIC
    raise TypeError(f"Unknown surface interpolation {interpolation!r}.")


def _require_built(curve, what: str) -> None:
    if not curve.is_ok():
        raise RuntimeError(f"{what}: {curve.error_messages()}")


def _discount_curve(sjd, anchor: date, currency: str, spec, connection: sqlite3.Connection):
    _require_linear(spec.interpolation, f"{currency} discount curve")
    pillars = discount_pillars(connection, anchor, currency)
    curve = sjd.DiscountCurve(
        anchor,
        [day for day, _factor in pillars],
        [factor for _day, factor in pillars],
        allow_extrapolation=spec.extrapolate,
    )
    _require_built(curve, f"{currency} discount curve")
    return curve


def _forward_curve(sjd, anchor, spec, connection, roll, volatility, catalogue):
    forward = spec.forward
    if isinstance(forward, GofoForward):
        dates, values = _gofo_outrights(sjd, anchor, spec, connection)
    elif isinstance(forward, OutrightForward):
        pillars = forward_pillars(connection, anchor, spec.market, spec.currency)
        dates = [day for day, _value in pillars]
        values = [value for _day, value in pillars]
    elif isinstance(forward, MetalTimesUsdZarForward):
        dates, values = _cross_outrights(sjd, anchor, spec, connection, catalogue)
    else:
        raise TypeError(f"No forward builder for {type(forward).__name__}.")

    options = {}
    if not isinstance(forward, MetalTimesUsdZarForward):
        options["allow_extrapolation"] = forward.extrapolate
        options["interpolation"] = _curve_interpolator(sjd, forward.interpolation)
    curve = sjd.ForwardCurve(
        anchor,
        dates,
        values,
        averaging=_sjd_averaging(sjd, forward.averaging),
        roll=_sjd_roll(sjd, roll),
        volatility=volatility,
        **options,
    )
    _require_built(curve, f"{spec.market} {spec.currency} forward")
    return curve


def _deterministic_forward(sjd, anchor, spec, connection, catalogue):
    return _forward_curve(
        sjd, anchor, spec, connection, ForwardRoll.DETERMINISTIC, None, catalogue
    )


def _gofo_outrights(sjd, anchor, spec, connection):
    """Convert each GOFO pillar to an outright, discounting in the quote currency.

    Between pillar dates the forward curve interpolates the outrights.
    """
    discount = _discount_curve(sjd, anchor, spec.discount_currency, spec.discount, connection)
    gold_spot = spot(connection, anchor, spec.market, spec.currency)
    settles = spot_date(spec, anchor)
    discount_spot = discount.get_discount_factor(settles)
    dates = []
    outrights = []
    for value_date, gofo in forward_pillars(connection, anchor, spec.market, spec.currency):
        year_fraction = (value_date - settles).days / 360.0
        discount_value = discount.get_discount_factor(value_date)
        outright = gold_spot * discount_spot / discount_value / (1.0 + gofo * year_fraction)
        dates.append(value_date)
        outrights.append(outright)
    return dates, outrights


def _cross_outrights(sjd, anchor, spec, connection, catalogue):
    """ZAR per unit of metal, sampled where either input curve has a pillar."""
    rule = spec.forward
    metal_spec = catalogue.quote_spec(rule.metal_market, rule.metal_currency)
    fx_spec = catalogue.quote_spec(rule.fx_market, rule.fx_currency)
    metal = _deterministic_forward(sjd, anchor, metal_spec, connection, catalogue)
    fx = _deterministic_forward(sjd, anchor, fx_spec, connection, catalogue)
    metal_pillars = forward_pillars(connection, anchor, rule.metal_market, rule.metal_currency)
    fx_pillars = forward_pillars(connection, anchor, rule.fx_market, rule.fx_currency)
    dates = sorted({day for day, _value in metal_pillars + fx_pillars})
    values = []
    for day in dates:
        value = metal.get_forward(day) * fx.get_forward(day)
        if not math.isfinite(value):
            raise RuntimeError(
                f"{spec.market} {spec.currency} forward is not finite on {day.isoformat()}."
            )
        values.append(value)
    return dates, values


def _volatility_surface(sjd, anchor, spec, connection):
    rule = spec.volatility
    dates, strikes, volatility = volatility_grid(
        connection, anchor, spec.market, spec.currency
    )
    interpolator = _surface_interpolator(sjd, rule.interpolation)
    if isinstance(rule, DeltaVol):
        surface = sjd.VolatilitySurfaceDelta(
            anchor,
            dates,
            strikes,
            volatility,
            interpolator,
            extrapolate=rule.extrapolate,
        )
    elif isinstance(rule, CalendarMonthMoneynessVol):
        surface = sjd.VolatilitySurfaceMoneynessCMA(
            anchor,
            dates,
            strikes,
            volatility,
            interpolator,
            extrapolate=rule.extrapolate,
        )
    else:
        raise TypeError(f"No volatility builder for {type(rule).__name__}.")
    _require_built(surface, f"{spec.market} {spec.currency} volatility")
    return surface
