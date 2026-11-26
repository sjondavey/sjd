"""Build a RatesEnvironment from stored pillars and the market spec."""

from __future__ import annotations

import calendar as calendar_module
import functools
import logging
import math
import sqlite3
from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from datetime import date, datetime, timedelta
from pathlib import Path
from typing import Any

from .schema import initialize
from .specs import (
    CATALOGUE,
    CalendarMonthDates,
    CalendarMonthMoneynessVol,
    CalendarSpec,
    Catalogue,
    CurveInterpolation,
    DeltaVol,
    FlatVol,
    ForwardAveraging,
    ForwardRoll,
    GofoForward,
    GofoRateForward,
    OutrightForward,
    PointDates,
    PointMoneynessVol,
    QuoteNotAvailable,
    QuoteSpec,
    SurfaceInterpolation,
    cross_markets,
    require_roll,
)
from .store import (
    RatesLabel,
    discount_pillars,
    ensure_label,
    forward_pillars,
    resolve_label,
    save_discount_pillars,
    save_forward_pillars,
    save_spot,
    save_volatility_pillars,
    spot,
    volatility_grid,
)

logger = logging.getLogger(__name__)


@dataclass(frozen=True)
class LoadedMarket:
    """Curves for one quote, and the environment that prices with them.

    ``forward``, ``discount``, and ``volatility`` are the objects stored on
    ``environment``. A strike ladder can be read against the outright and the
    discount factor. The environment prices a contract.
    """

    spec: QuoteSpec
    anchor: date
    forward: Any
    discount: Any
    volatility: Any
    environment: Any
    outrights: tuple[tuple[date, float], ...] = ()
    fixing: Any = None
    label: RatesLabel | None = None

    def calendar(self):
        """The quote's calendar. A contract month uses its publication calendar."""
        dates = self.spec.dates
        if isinstance(dates, PointDates):
            return calendar(dates.calendar)
        if isinstance(dates, CalendarMonthDates):
            return calendar(dates.publication_calendar)
        raise TypeError(f"No calendar for {type(dates).__name__}.")

    def spot_date(self) -> date:
        """The quote's spot date."""
        return spot_date(self.spec, self.anchor)

    def option_expiry(self, day: date) -> date:
        """Expiry of an option that settles on ``day``."""
        return option_expiry(self.spec, day)

    def pricing_dates(self, day: date) -> list[date]:
        """Publication business days in the contract month that contains ``day``."""
        return pricing_dates(self.spec, day)

    def month_settlement(self, day: date) -> date:
        """Cash settlement for the contract month that contains ``day``."""
        return month_settlement(self.spec, day)

    def with_roll(self, roll: ForwardRoll) -> LoadedMarket:
        """The same curves with a different forward roll.

        A deterministic market prices. ``FORWARD_VOLATILITY`` or
        ``SPOT_VOLATILITY`` is what a Monte Carlo needs later.
        """
        require_roll(self.spec.forward.averaging, roll, f"{self.spec.market} {self.spec.currency} forward")
        sjd = _sjd()
        dates = [day for day, _value in self.outrights]
        values = [value for _day, value in self.outrights]
        curve = _make_forward_curve(sjd, self.anchor, self.spec, dates, values, roll, self.volatility)
        environment = sjd.RatesEnvironment(self.anchor)
        environment.set_frs(curve)
        environment.set_drs(self.discount)
        environment.set_vrs(self.volatility)
        if self.fixing is not None:
            environment.set_fixing_rs(self.fixing)
        logger.info(
            "loaded market %s/%s anchor %s roll %s",
            self.spec.market,
            self.spec.currency,
            self.anchor.isoformat(),
            roll.name,
        )
        return LoadedMarket(
            spec=self.spec,
            anchor=self.anchor,
            forward=curve,
            discount=self.discount,
            volatility=self.volatility,
            environment=environment,
            outrights=self.outrights,
            fixing=self.fixing,
            label=self.label,
        )

    def monthly_schedule(self, start: date, months: int, convention=None):
        """One settlement a month, adjusted on this quote's calendar."""
        from sjd.structuring import Schedule

        return Schedule.monthly(self, start, months, convention)


def load_market(
    market: str,
    anchor: date,
    currency: str,
    connection: sqlite3.Connection,
    roll: ForwardRoll | None = None,
    catalogue: Catalogue | None = None,
    *,
    label: str | None = None,
    as_of: datetime | str | None = None,
) -> LoadedMarket:
    """Build the curves for one quote and the environment that holds them.

    ``roll`` replaces the quote's default ForwardRoll. A DETERMINISTIC
    environment prices but cannot drive a Monte Carlo. FORWARD_VOLATILITY and
    SPOT_VOLATILITY simulate the forward with the quote's own surface, which
    is also the environment's volatility.

    ``label`` selects a rates label. Omit ``as_of`` for its newest instance.
    Without a label, ``anchor`` must belong to exactly one label. The curve
    date is that label's anchor date.

    ``catalogue`` defaults to ``catalogue.toml``.
    """
    sjd = _sjd()
    catalogue = catalogue or CATALOGUE
    stored = resolve_label(connection, name=label, as_of=as_of, anchor=anchor)
    anchor = stored.anchor
    spec = catalogue.quote_spec(market, currency)
    for calendar_spec in _calendar_specs(spec):
        calendar(calendar_spec)
    if roll is None:
        roll = spec.forward.roll
    require_roll(spec.forward.averaging, roll, f"{spec.market} {spec.currency} forward")
    discount = _discount_curve(sjd, stored, spec.discount_currency, spec.discount, connection)
    volatility = _volatility_surface(sjd, stored, spec, connection)
    forward, outright_dates, outright_values = _forward_curve(
        sjd, stored, spec, connection, roll, volatility, catalogue
    )
    environment = sjd.RatesEnvironment(anchor)
    environment.set_frs(forward)
    environment.set_drs(discount)
    environment.set_vrs(volatility)
    fixing = _fixing_curve(sjd, anchor, spec, outright_values)
    if fixing is not None:
        environment.set_fixing_rs(fixing)
    logger.info(
        "loaded market %s/%s label %s at %s anchor %s roll %s",
        market,
        currency,
        stored.name,
        stored.as_of_text,
        anchor.isoformat(),
        roll.name,
    )
    return LoadedMarket(
        spec=spec,
        anchor=anchor,
        forward=forward,
        discount=discount,
        volatility=volatility,
        environment=environment,
        outrights=tuple(zip(outright_dates, outright_values, strict=True)),
        fixing=fixing,
        label=stored,
    )


def open_market(
    source,
    market: str,
    currency: str = "USD",
    anchor: date | None = None,
    roll: ForwardRoll | None = None,
    catalogue: Catalogue | None = None,
    *,
    label: str | None = None,
    as_of: datetime | str | None = None,
) -> LoadedMarket:
    """Load a quote from a pillar database.

    ``source`` is a path or an open SQLite connection. ``label`` selects the
    rates label, and the newest instance of that name when ``as_of`` is
    omitted. ``as_of`` is a datetime or an ISO-8601 string. Without a label,
    the file must hold one label, or one label on ``anchor``.
    """
    opened = _connect(source)
    try:
        stored = resolve_label(opened, name=label, as_of=as_of, anchor=anchor)
        return load_market(
            market,
            stored.anchor,
            currency,
            opened,
            roll,
            catalogue,
            label=stored.name,
            as_of=stored.as_of,
        )
    finally:
        if opened is not source:
            opened.close()


def save_market_pillars(
    target,
    *,
    label: str,
    as_of: datetime | str,
    anchor: date,
    market: str,
    currency: str = "USD",
    spot: float | None = None,
    discount,
    forward=None,
    volatility,
    roll: ForwardRoll | None = None,
    catalogue: Catalogue | None = None,
) -> LoadedMarket:
    """Write one quote's pillars onto a rates label and return the market.

    ``label`` and ``as_of`` identify the snapshot. ``as_of`` is a datetime or
    an ISO-8601 string. ``anchor`` is the curve date. A label that already
    exists keeps its anchor, and a different anchor is refused, so every
    instrument on the label shares one curve date. Another commodity is the
    same label and the same anchor. A later snapshot is a new ``as_of``.

    ``target`` is a path or an open SQLite connection. Discount pillars for
    ``currency`` on this label are replaced, and so are this quote's forward
    and volatility pillars. ``spot`` is required for a GOFO or lease quote.
    """
    catalogue = catalogue or CATALOGUE
    connection = _connect(target)
    try:
        stored = _write_quote(
            connection, catalogue, label, as_of, anchor, market, currency, spot, discount, forward, volatility
        )
        connection.commit()
        return load_market(
            market, stored.anchor, currency, connection, roll, catalogue, label=stored.name, as_of=stored.as_of
        )
    finally:
        if connection is not target:
            connection.close()


def build_market(
    market: str,
    anchor: date,
    currency: str = "USD",
    *,
    spot: float | None = None,
    discount,
    forward=None,
    volatility,
    roll: ForwardRoll | None = None,
    catalogue: Catalogue | None = None,
    also: Sequence[Mapping] = (),
) -> LoadedMarket:
    """Build a market from pillars without keeping a database file.

    ``also`` holds extra quotes a cross needs. Each item has ``market``,
    ``currency``, and any of ``spot``, ``discount``, ``forward``, and
    ``volatility``. The last discount written for a currency is the one that
    is stored.
    """
    catalogue = catalogue or CATALOGUE
    connection = sqlite3.connect(":memory:")
    try:
        stamp = datetime.combine(anchor, datetime.min.time())
        _write_quote(
            connection, catalogue, "build", stamp, anchor, market, currency, spot, discount, forward, volatility
        )
        for extra in also:
            _write_quote(
                connection,
                catalogue,
                "build",
                stamp,
                anchor,
                extra["market"],
                extra["currency"],
                extra.get("spot"),
                extra.get("discount"),
                extra.get("forward"),
                extra.get("volatility"),
            )
        connection.commit()
        return load_market(market, anchor, currency, connection, roll, catalogue, label="build", as_of=stamp)
    finally:
        connection.close()


def _connect(source):
    if isinstance(source, (str, Path)):
        path = Path(source)
        path.parent.mkdir(parents=True, exist_ok=True)
        return sqlite3.connect(path)
    return source


def _write_quote(
    connection, catalogue, label, as_of, anchor, market, currency, spot_value, discount, forward, volatility
):
    connection.execute("PRAGMA foreign_keys = ON")
    initialize(connection, catalogue)
    stored = ensure_label(connection, label, as_of, anchor)
    if spot_value is not None:
        save_spot(connection, stored, market, currency, spot_value)
    if discount is not None:
        save_discount_pillars(connection, stored, currency, discount)
    if forward is not None:
        save_forward_pillars(connection, stored, market, currency, forward)
    if volatility is not None:
        save_volatility_pillars(connection, stored, market, currency, volatility)
    return stored


def rates_environment(
    market: str,
    anchor: date,
    currency: str,
    connection: sqlite3.Connection,
    roll: ForwardRoll | None = None,
    catalogue: Catalogue | None = None,
    *,
    label: str | None = None,
    as_of: datetime | str | None = None,
):
    """Forward, discount, and volatility for one quote, ready to price.

    Returns the environment. ``load_market`` returns that environment and the
    curves on it. The arguments are the same. ``label`` selects a rates label,
    and the newest instance of that name when ``as_of`` is omitted.
    """
    return load_market(
        market, anchor, currency, connection, roll, catalogue, label=label, as_of=as_of
    ).environment


@functools.cache
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
    import sjd

    try:
        sjd.require_valid(curve)
    except sjd.ValidationError as error:
        raise sjd.ValidationError(f"{what}\n{error}", error.report) from None


def _discount_curve(sjd, stored: RatesLabel, currency: str, spec, connection: sqlite3.Connection):
    _require_linear(spec.interpolation, f"{currency} discount curve")
    pillars = discount_pillars(
        connection, stored.anchor, currency, label=stored.name, as_of=stored.as_of
    )
    curve = sjd.DiscountCurve(
        stored.anchor,
        [day for day, _factor in pillars],
        [factor for _day, factor in pillars],
        allow_extrapolation=spec.extrapolate,
    )
    _require_built(curve, f"{currency} discount curve")
    return curve


def _fixing_curve(sjd, anchor: date, spec: QuoteSpec, outright_values: Sequence[float]):
    """A historical print so an Asian on a calendar-month quote can be priced.

    Dates after the anchor are read from the forward curve. This curve only
    covers a print before the anchor.
    """
    if not isinstance(spec.dates, CalendarMonthDates) or not outright_values:
        return None
    return sjd.DailyFixingCurve([anchor - timedelta(days=1)], [float(outright_values[0])])


def _make_forward_curve(sjd, anchor, spec, dates, values, roll, volatility):
    forward = spec.forward
    options = {}
    if cross_markets(forward) is None:
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


def _forward_curve(sjd, stored: RatesLabel, spec, connection, roll, volatility, catalogue):
    forward = spec.forward
    if isinstance(forward, (GofoForward, GofoRateForward)):
        dates, values = _gofo_outrights(sjd, stored, spec, connection)
    elif isinstance(forward, OutrightForward):
        pillars = forward_pillars(
            connection, stored.anchor, spec.market, spec.currency, label=stored.name, as_of=stored.as_of
        )
        dates = [day for day, _value in pillars]
        values = [value for _day, value in pillars]
    elif cross_markets(forward) is not None:
        dates, values = _cross_outrights(sjd, stored, spec, connection, catalogue)
    else:
        raise TypeError(f"No forward builder for {type(forward).__name__}.")
    return _make_forward_curve(sjd, stored.anchor, spec, dates, values, roll, volatility), dates, values


def _deterministic_forward(sjd, stored: RatesLabel, spec, connection, catalogue):
    curve, _dates, _values = _forward_curve(
        sjd, stored, spec, connection, ForwardRoll.DETERMINISTIC, None, catalogue
    )
    return curve


def _gofo_outrights(sjd, stored: RatesLabel, spec, connection):
    """Convert each stored gold rate to an outright.

    A lease rate is discounted. A GOFO rate is ``spot * (1 + rate * tau)``.
    Between pillar dates the forward curve interpolates the outrights.
    """
    rate_is_gofo = isinstance(spec.forward, GofoRateForward)
    discount = None if rate_is_gofo else _discount_curve(
        sjd, stored, spec.discount_currency, spec.discount, connection
    )
    gold_spot = spot(connection, stored.anchor, spec.market, spec.currency, label=stored.name, as_of=stored.as_of)
    settles = spot_date(spec, stored.anchor)
    discount_spot = None if discount is None else discount.get_discount_factor(settles)
    dates = []
    outrights = []
    for value_date, rate in forward_pillars(
        connection, stored.anchor, spec.market, spec.currency, label=stored.name, as_of=stored.as_of
    ):
        year_fraction = (value_date - settles).days / 360.0
        if rate_is_gofo:
            outright = gold_spot * (1.0 + rate * year_fraction)
        else:
            assert discount is not None and discount_spot is not None
            discount_value = discount.get_discount_factor(value_date)
            outright = gold_spot * discount_spot / discount_value / (1.0 + rate * year_fraction)
        dates.append(value_date)
        outrights.append(outright)
    return dates, outrights


def _cross_outrights(sjd, stored: RatesLabel, spec, connection, catalogue):
    """Product of two outrights, sampled where either input curve has a pillar."""
    crossed = cross_markets(spec.forward)
    if crossed is None:
        raise TypeError(f"No cross builder for {type(spec.forward).__name__}.")
    base_market, base_currency, fx_market, fx_currency = crossed
    metal_spec = catalogue.quote_spec(base_market, base_currency)
    fx_spec = catalogue.quote_spec(fx_market, fx_currency)
    metal = _deterministic_forward(sjd, stored, metal_spec, connection, catalogue)
    fx = _deterministic_forward(sjd, stored, fx_spec, connection, catalogue)
    metal_pillars = forward_pillars(
        connection, stored.anchor, base_market, base_currency, label=stored.name, as_of=stored.as_of
    )
    fx_pillars = forward_pillars(
        connection, stored.anchor, fx_market, fx_currency, label=stored.name, as_of=stored.as_of
    )
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


def _volatility_surface(sjd, stored: RatesLabel, spec, connection):
    rule = spec.volatility
    dates, strikes, volatility = volatility_grid(
        connection, stored.anchor, spec.market, spec.currency, label=stored.name, as_of=stored.as_of
    )
    if isinstance(rule, FlatVol):
        if len(strikes) != 1:
            raise ValueError(
                f"{spec.market} {spec.currency} flat volatility has {len(strikes)} strike rows. "
                "Store one strike dimension, 0."
            )
        curve_type = (
            sjd.VolatilityCurveCMA
            if spec.forward.averaging is ForwardAveraging.CALENDAR_MONTH
            else sjd.VolatilityCurve
        )
        surface = curve_type(stored.anchor, dates, volatility[0], allow_extrapolation=rule.extrapolate)
    elif isinstance(rule, (DeltaVol, PointMoneynessVol, CalendarMonthMoneynessVol)):
        interpolator = _surface_interpolator(sjd, rule.interpolation)
        if isinstance(rule, DeltaVol):
            surface_type = sjd.VolatilitySurfaceDelta
        elif isinstance(rule, PointMoneynessVol):
            surface_type = sjd.VolatilitySurfaceMoneyness
        else:
            surface_type = sjd.VolatilitySurfaceMoneynessCMA
        surface = surface_type(
            stored.anchor,
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
