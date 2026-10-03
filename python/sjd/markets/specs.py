"""Market convention types, and the loader for ``catalogue.toml``.

A spec names the curve family, the calendar, and where the forward number
comes from. It does not build a RatesEnvironment. Pricing still uses the
generic forward, volatility, and European contracts.

The markets themselves are data in ``catalogue.toml``. ``CATALOGUE`` is that
file, loaded once on import. ``load_catalogue`` and ``parse_catalogue`` read
another file or string into the same types.

Every forward has a ForwardAveraging and a default ForwardRoll. A stored
forward also names a CurveInterpolation and whether it may extrapolate. A
stored surface names a SurfaceInterpolation and the same extrapolation
flag. Each quote sets its own, so one market can change without moving the
others. A metal-times-USDZAR forward is a product of two curves and has
neither an interpolator nor an extrapolation flag.

Each currency has one discount curve, shared by every quote that settles in
that currency.
"""

from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Any, Mapping, Optional, Union

try:
    import tomllib
except ModuleNotFoundError:  # Python 3.10
    import tomli as tomllib


CATALOGUE_PATH = Path(__file__).with_name("catalogue.toml")


class QuoteNotAvailable(KeyError):
    """The market has no default environment in that currency."""


class CatalogueError(ValueError):
    """The catalogue file does not describe a valid set of markets."""


class CurveInterpolation(Enum):
    """Time interpolation for a stored forward curve. Matches ArrayInterpolatorType."""

    LINEAR = "LINEAR"
    CUBIC = "CUBIC"


class SurfaceInterpolation(Enum):
    """Interpolation for a stored volatility surface. Matches SurfaceInterpolatorType."""

    BILINEAR = "BILINEAR"
    BICUBIC = "BICUBIC"


class ForwardAveraging(Enum):
    """How a forward curve reads a date. Matches sjd.ForwardAveraging.

    CALENDAR_MONTH returns the same forward for every day in a month.
    """

    POINT = "POINT"
    CALENDAR_MONTH = "CALENDAR_MONTH"


class ForwardRoll(Enum):
    """How a forward curve moves through a simulation. Matches sjd.ForwardRoll.

    A Monte Carlo needs FORWARD_VOLATILITY or SPOT_VOLATILITY. Both read the
    quote's own surface. A calendar-month forward has no spot-volatility roll.
    """

    DETERMINISTIC = "DETERMINISTIC"
    FORWARD_VOLATILITY = "FORWARD_VOLATILITY"
    SPOT_VOLATILITY = "SPOT_VOLATILITY"


@dataclass(frozen=True)
class CalendarSpec:
    """A holiday calendar, by a name from ``sjd.Calendar.names()``.

    With ``components``, ``name`` is only a label and the calendar is closed
    whenever any component is closed.
    """

    name: str
    components: tuple[str, ...] = ()


@dataclass(frozen=True)
class PointDates:
    """Spot, then a value date. Option expiry sits before that value date."""

    calendar: CalendarSpec
    spot_lag_business_days: int
    expiry_before_value_business_days: int


@dataclass(frozen=True)
class CalendarMonthDates:
    """Read the forward and the vol inside the contract month.

    A calendar-month curve returns the average of the month you ask for, so
    the lookup date has to stay in that month. Cash settles later, and that
    later date is only for the discount factor.
    """

    publication_calendar: CalendarSpec
    settlement_calendar: CalendarSpec
    settlement_lag_business_days: int


@dataclass(frozen=True)
class GofoForward:
    """GOFO lease rates, turned into an outright before the curve is built.

    ``gofo`` is a simple Actual/360 rate from spot to the value date.
    ``discount_usd`` is the USD discount factor from the anchor date.

    outright(value) = spot
        * discount_usd(spot) / discount_usd(value)
        / (1 + gofo * actual360(spot, value))

    The outrights are a point curve. ``interpolation`` runs between them.
    """

    interpolation: CurveInterpolation = CurveInterpolation.LINEAR
    extrapolate: bool = False
    roll: ForwardRoll = ForwardRoll.DETERMINISTIC

    @property
    def averaging(self) -> ForwardAveraging:
        return ForwardAveraging.POINT

    def __post_init__(self) -> None:
        _require_curve_interpolation(self.interpolation)
        _require_bool(self.extrapolate, "Forward extrapolation")
        require_roll(self.averaging, self.roll)


@dataclass(frozen=True)
class OutrightForward:
    """Pillars that are already the forward the contract will look up.

    ``interpolation`` runs between those pillars. On a calendar-month curve
    the pillars are contract months.
    """

    averaging: ForwardAveraging
    interpolation: CurveInterpolation = CurveInterpolation.LINEAR
    extrapolate: bool = False
    roll: ForwardRoll = ForwardRoll.DETERMINISTIC

    def __post_init__(self) -> None:
        if not isinstance(self.averaging, ForwardAveraging):
            raise TypeError(
                "Forward averaging must be a ForwardAveraging "
                f"(POINT or CALENDAR_MONTH), got {self.averaging!r}."
            )
        _require_curve_interpolation(self.interpolation)
        _require_bool(self.extrapolate, "Forward extrapolation")
        require_roll(self.averaging, self.roll)


@dataclass(frozen=True)
class MetalTimesUsdZarForward:
    """ZAR per unit of metal, from two outright curves.

    forward(t) = metal_usd_forward(t) * usdzar_forward(t)

    ``t`` is the metal value date. The product is built once, on the anchor
    date, and then rolls as one curve with the quote's own ZAR surface.
    Each input curve interpolates and extrapolates itself. This product does neither.
    """

    metal_market: str
    metal_currency: str = "USD"
    fx_market: str = "USDZAR"
    fx_currency: str = "ZAR"
    roll: ForwardRoll = ForwardRoll.DETERMINISTIC

    @property
    def averaging(self) -> ForwardAveraging:
        return ForwardAveraging.POINT

    def __post_init__(self) -> None:
        require_roll(self.averaging, self.roll)


@dataclass(frozen=True)
class DeltaVol:
    """Stored delta surface.

    Rows are Black forward put deltas in percent, with 50 at the money.
    Volatility becomes a standard deviation on Actual/365 Fixed. Both are
    fixed by VolatilitySurfaceDelta. Deltas outside the grid read the wing.
    """

    interpolation: SurfaceInterpolation = SurfaceInterpolation.BILINEAR
    extrapolate: bool = False

    def __post_init__(self) -> None:
        _require_surface_interpolation(self.interpolation)
        _require_bool(self.extrapolate, "Surface extrapolation")


@dataclass(frozen=True)
class CalendarMonthMoneynessVol:
    """Stored calendar-month moneyness surface.

    Rows are (strike - forward) / forward. Moneyness outside the grid reads
    the wing. ``extrapolate`` defaults to True because that is the
    constructor default on VolatilitySurfaceMoneynessCMA. A forward curve
    still defaults to False.
    """

    interpolation: SurfaceInterpolation = SurfaceInterpolation.BILINEAR
    extrapolate: bool = True

    def __post_init__(self) -> None:
        _require_surface_interpolation(self.interpolation)
        _require_bool(self.extrapolate, "Surface extrapolation")


@dataclass(frozen=True)
class DiscountCurveSpec:
    """Stored discount factors from the anchor date. One spec per currency."""

    interpolation: CurveInterpolation = CurveInterpolation.LINEAR
    extrapolate: bool = False

    def __post_init__(self) -> None:
        _require_curve_interpolation(self.interpolation)
        _require_bool(self.extrapolate, "Discount extrapolation")


ForwardRule = Union[GofoForward, OutrightForward, MetalTimesUsdZarForward]
VolRule = Union[DeltaVol, CalendarMonthMoneynessVol]
DateRule = Union[PointDates, CalendarMonthDates]


@dataclass(frozen=True)
class QuoteSpec:
    """One default environment: market plus settlement currency."""

    market: str
    currency: str
    forward: ForwardRule
    volatility: VolRule
    discount_currency: str
    discount: DiscountCurveSpec
    dates: DateRule

    @property
    def stores_forward(self) -> bool:
        return not isinstance(self.forward, MetalTimesUsdZarForward)

    @property
    def stores_spot(self) -> bool:
        return isinstance(self.forward, GofoForward)


@dataclass(frozen=True)
class MarketSpec:
    market: str
    summary: str
    quotes: Mapping[str, QuoteSpec]

    def in_currency(self, currency: str) -> QuoteSpec:
        key = currency.upper()
        try:
            return self.quotes[key]
        except KeyError:
            allowed = ", ".join(self.quotes)
            raise QuoteNotAvailable(
                f"{self.market} has no {key} quote. Available: {allowed}."
            ) from None


@dataclass(frozen=True)
class Catalogue:
    """Every market, and the discount curve for each settlement currency."""

    markets: Mapping[str, MarketSpec]
    discount_curves: Mapping[str, DiscountCurveSpec]

    def market_spec(self, market: str) -> MarketSpec:
        key = market.upper()
        try:
            return self.markets[key]
        except KeyError:
            known = ", ".join(self.markets)
            raise QuoteNotAvailable(f"Unknown market {key}. Known: {known}.") from None

    def quote_spec(self, market: str, currency: str) -> QuoteSpec:
        return self.market_spec(market).in_currency(currency)

    def quotes(self) -> list[QuoteSpec]:
        return [quote for market in self.markets.values() for quote in market.quotes.values()]


def _require_curve_interpolation(interpolation: CurveInterpolation) -> None:
    if not isinstance(interpolation, CurveInterpolation):
        raise TypeError(
            "Forward interpolation must be a CurveInterpolation "
            f"(LINEAR or CUBIC), got {interpolation!r}."
        )


def _require_surface_interpolation(interpolation: SurfaceInterpolation) -> None:
    if not isinstance(interpolation, SurfaceInterpolation):
        raise TypeError(
            "Surface interpolation must be a SurfaceInterpolation "
            f"(BILINEAR or BICUBIC), got {interpolation!r}."
        )


def _require_bool(value: bool, label: str) -> None:
    if not isinstance(value, bool):
        raise TypeError(f"{label} must be True or False, got {value!r}.")


def require_roll(averaging: ForwardAveraging, roll: ForwardRoll, what: str = "Forward") -> None:
    """Raise unless ``roll`` is a ForwardRoll that ``averaging`` can use."""
    if not isinstance(roll, ForwardRoll):
        raise TypeError(
            f"{what} roll must be a ForwardRoll "
            f"(DETERMINISTIC, FORWARD_VOLATILITY or SPOT_VOLATILITY), got {roll!r}."
        )
    if averaging is ForwardAveraging.CALENDAR_MONTH and roll is ForwardRoll.SPOT_VOLATILITY:
        raise ValueError(f"{what} averages by calendar month and has no spot-volatility roll.")


_FORWARD_KINDS = {
    "gofo": GofoForward,
    "outright": OutrightForward,
    "metal_times_usdzar": MetalTimesUsdZarForward,
}
_VOLATILITY_KINDS = {
    "delta": DeltaVol,
    "calendar_month_moneyness": CalendarMonthMoneynessVol,
}
_DATE_KINDS = {
    "point": PointDates,
    "calendar_month": CalendarMonthDates,
}
_FORWARD_ENUMS = {
    "interpolation": CurveInterpolation,
    "averaging": ForwardAveraging,
    "roll": ForwardRoll,
}
_SECTIONS = {"calendars", "dates", "discount", "markets"}


def load_catalogue(path: Union[str, Path] = CATALOGUE_PATH) -> Catalogue:
    """Read a catalogue file. See ``catalogue.toml`` for the layout."""
    path = Path(path)
    try:
        return parse_catalogue(path.read_text(encoding="utf-8"))
    except CatalogueError as error:
        raise CatalogueError(f"{path.name}: {error}") from None


def parse_catalogue(text: str) -> Catalogue:
    """Read catalogue TOML. Unknown keys and unknown names are errors."""
    try:
        data = tomllib.loads(text)
    except tomllib.TOMLDecodeError as error:
        raise CatalogueError(str(error)) from None
    _require_known_keys(data, _SECTIONS, "The catalogue")

    calendars = {
        name: _build(CalendarSpec, table, f"calendars.{name}")
        for name, table in data.get("calendars", {}).items()
    }
    calendar_refs = {
        "calendar": calendars,
        "publication_calendar": calendars,
        "settlement_calendar": calendars,
    }
    dates = {}
    for name, table in data.get("dates", {}).items():
        where = f"dates.{name}"
        kind, fields = _kind(table, _DATE_KINDS, where)
        dates[name] = _build(kind, fields, where, refs=calendar_refs)
    discount_curves = {}
    for currency, table in data.get("discount", {}).items():
        _require_upper(currency, "discount")
        discount_curves[currency] = _build(
            DiscountCurveSpec,
            table,
            f"discount.{currency}",
            enums={"interpolation": CurveInterpolation},
        )

    markets = {}
    for market, table in data.get("markets", {}).items():
        where = f"markets.{market}"
        _require_upper(market, "markets")
        _require_known_keys(table, {"summary", "quotes"}, where)
        quotes = {}
        for currency, quote_table in table.get("quotes", {}).items():
            _require_upper(currency, f"{where}.quotes")
            quotes[currency] = _quote(
                market,
                currency,
                quote_table,
                dates,
                discount_curves,
                f"{where}.quotes.{currency}",
            )
        if not quotes:
            raise CatalogueError(f"{where} has no quotes.")
        markets[market] = MarketSpec(
            market=market, summary=table.get("summary", ""), quotes=quotes
        )

    catalogue = Catalogue(markets=markets, discount_curves=discount_curves)
    _check_catalogue(catalogue)
    return catalogue


def _quote(
    market: str,
    currency: str,
    table: Mapping[str, Any],
    dates: Mapping[str, DateRule],
    discount_curves: Mapping[str, DiscountCurveSpec],
    where: str,
) -> QuoteSpec:
    _require_known_keys(table, {"dates", "forward", "volatility"}, where)
    for key in ("dates", "forward", "volatility"):
        if key not in table:
            raise CatalogueError(f"{where} needs '{key}'.")
    forward_kind, forward_fields = _kind(table["forward"], _FORWARD_KINDS, f"{where}.forward")
    forward = _build(forward_kind, forward_fields, f"{where}.forward", enums=_FORWARD_ENUMS)
    vol_kind, vol_fields = _kind(table["volatility"], _VOLATILITY_KINDS, f"{where}.volatility")
    volatility = _build(
        vol_kind,
        vol_fields,
        f"{where}.volatility",
        enums={"interpolation": SurfaceInterpolation},
    )
    if currency not in discount_curves:
        raise CatalogueError(f"{where} settles in {currency}, which has no [discount.{currency}].")
    return QuoteSpec(
        market=market,
        currency=currency,
        forward=forward,
        volatility=volatility,
        discount_currency=currency,
        discount=discount_curves[currency],
        dates=_lookup(dates, table["dates"], f"{where}.dates"),
    )


def _kind(table: Mapping[str, Any], kinds: Mapping[str, type], where: str):
    fields = dict(table)
    name = fields.pop("kind", None)
    if name not in kinds:
        allowed = ", ".join(kinds)
        raise CatalogueError(f"{where}.kind must be one of {allowed}, got {name!r}.")
    return kinds[name], fields


def _build(
    kind: type,
    table: Mapping[str, Any],
    where: str,
    enums: Optional[Mapping[str, type]] = None,
    refs: Optional[Mapping[str, Mapping[str, Any]]] = None,
):
    fields = {}
    for key, value in table.items():
        if enums and key in enums:
            value = _enum_member(enums[key], value, f"{where}.{key}")
        elif refs and key in refs:
            value = _lookup(refs[key], value, f"{where}.{key}")
        elif isinstance(value, list):
            value = tuple(value)
        fields[key] = value
    try:
        return kind(**fields)
    except (TypeError, ValueError) as error:
        raise CatalogueError(f"{where}: {error}") from None


def _enum_member(enum: type, value: Any, where: str):
    try:
        return enum[value]
    except (KeyError, TypeError):
        allowed = ", ".join(member.name for member in enum)
        raise CatalogueError(f"{where} must be one of {allowed}, got {value!r}.") from None


def _lookup(table: Mapping[str, Any], name: Any, where: str):
    try:
        return table[name]
    except (KeyError, TypeError):
        known = ", ".join(table) or "none"
        raise CatalogueError(f"{where} names {name!r}, which is not defined. Known: {known}.") from None


def _require_known_keys(table: Mapping[str, Any], allowed: set, where: str) -> None:
    unknown = sorted(set(table) - allowed)
    if unknown:
        raise CatalogueError(f"{where} has unknown keys: {', '.join(unknown)}.")


def _require_upper(name: str, where: str) -> None:
    if name != name.upper():
        raise CatalogueError(f"{where}.{name} must be upper case.")


def _check_catalogue(catalogue: Catalogue) -> None:
    for quote in catalogue.quotes():
        where = f"markets.{quote.market}.quotes.{quote.currency}"
        forward = quote.forward
        monthly = forward.averaging is ForwardAveraging.CALENDAR_MONTH
        if monthly != isinstance(quote.dates, CalendarMonthDates):
            raise CatalogueError(
                f"{where} averages by {forward.averaging.name} "
                f"but its dates are {type(quote.dates).__name__}."
            )
        if monthly != isinstance(quote.volatility, CalendarMonthMoneynessVol):
            raise CatalogueError(
                f"{where} averages by {forward.averaging.name} "
                f"but its surface is {type(quote.volatility).__name__}."
            )
        if not isinstance(forward, MetalTimesUsdZarForward):
            continue
        try:
            inputs = [
                catalogue.quote_spec(forward.metal_market, forward.metal_currency),
                catalogue.quote_spec(forward.fx_market, forward.fx_currency),
            ]
        except QuoteNotAvailable as error:
            raise CatalogueError(f"{where} crosses a missing quote. {error.args[0]}") from None
        for source in inputs:
            if not source.stores_forward:
                raise CatalogueError(f"{where} crosses curves that are themselves derived.")
            if source.forward.averaging is not ForwardAveraging.POINT:
                raise CatalogueError(f"{where} can only cross point forwards.")


CATALOGUE = load_catalogue()
MARKETS = CATALOGUE.markets
DISCOUNT_CURVES = CATALOGUE.discount_curves


def market_spec(market: str) -> MarketSpec:
    """Return the spec for ``market`` from ``catalogue.toml``."""
    return CATALOGUE.market_spec(market)


def quote_spec(market: str, currency: str) -> QuoteSpec:
    """Return the default environment for one market and settlement currency."""
    return CATALOGUE.quote_spec(market, currency)
