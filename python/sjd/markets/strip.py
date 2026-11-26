"""Strip deposit curves, gold forwards, and broker smiles.

QuantLib builds the discount curve. Install the ``rates`` extra for that.
The tenor helpers and the smile conversion do not need it.
"""

from __future__ import annotations

from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from datetime import date
from typing import Any, cast

import sjd
from sjd.markets.rates import spot_date
from sjd.markets.specs import QuoteSpec

_TENOR_UNITS = {
    "D": "DAYS",
    "W": "WEEKS",
    "M": "MONTHS",
    "Y": "YEARS",
}


def parse_tenor(label: str) -> tuple[int, sjd.TimeUnit]:
    """Read ``1W``, ``3M``, or ``5Y`` as a count and a ``sjd.TimeUnit``."""
    text = str(label).strip().upper()
    if len(text) < 2 or text[-1] not in _TENOR_UNITS or not text[:-1].isdigit():
        raise ValueError(f"tenor must look like 1W, 3M, or 5Y, got {label!r}")
    unit = getattr(sjd.TimeUnit, _TENOR_UNITS[text[-1]])
    return int(text[:-1]), unit


def advance_tenor(calendar, day: date, label: str, convention=None, end_of_month: bool = False) -> date:
    """Move ``label`` on ``calendar``, then adjust by ``convention``."""
    count, unit = parse_tenor(label)
    if convention is None:
        convention = sjd.BusinessDayConvention.MODIFIED_FOLLOWING
    return calendar.advance(day, count, unit, convention, end_of_month)


def wing_vols(atm: float, rr: float, bf: float) -> tuple[float, float]:
    """Put and call wing vols from an at-the-money vol, risk reversal, and butterfly.

    The risk reversal is call vol minus put vol. The butterfly is the average
    of the two wings minus the at-the-money vol.
    """
    return atm + bf - 0.5 * rr, atm + bf + 0.5 * rr


@dataclass(frozen=True)
class StrippedDiscountCurve:
    """A QuantLib discount curve and the pillar dates worth storing."""

    anchor: date
    pillars: tuple[tuple[date, float], ...]
    _curve: Any

    def df(self, day: date) -> float:
        """Discount factor from the anchor to ``day``.

        QuantLib rebuilds a relative curve from ``Settings.evaluationDate``, so
        the call puts that date back on the anchor and then restores it.
        """
        ql = _quantlib()
        settings = ql.Settings.instance()
        previous = settings.evaluationDate
        settings.evaluationDate = _ql_date(ql, self.anchor)
        try:
            return float(self._curve.discount(_ql_date(ql, day)))
        finally:
            settings.evaluationDate = previous

    def pillars_for(self, extra_dates: Sequence[date] = ()) -> dict[date, float]:
        """Discount factors on the anchor, the stripped pillars, and ``extra_dates``.

        Saving these dates keeps a linearly interpolated discount curve on the
        same values as this curve.
        """
        found = {self.anchor: self.df(self.anchor)}
        for day, factor in self.pillars:
            found[day] = factor
        for day in extra_dates:
            found[day] = self.df(day)
        return found


@dataclass(frozen=True)
class StrippedLease:
    """Lease rates implied by GOFO, plus the discount dates that reproduce them."""

    spot_date: date
    rows: tuple[dict, ...]
    lease_pillars: tuple[tuple[date, float], ...]
    discount_pillars: dict[date, float]

    def frame(self):
        """The rows as a pandas DataFrame."""
        pandas = _pandas()
        return pandas.DataFrame(self.rows)


@dataclass(frozen=True)
class StrippedSmile:
    """Put-delta pillars, including the anchor smile when one was requested."""

    pillars: tuple[tuple[str, date, dict[float, float]], ...]
    deltas: tuple[float, ...]

    def rows(self) -> list[tuple[date, float, float]]:
        """``(date, delta, volatility)`` rows for the pillar store."""
        stored = []
        for _label, day, smile in self.pillars:
            for delta in self.deltas:
                stored.append((day, delta, smile[delta]))
        return stored

    def frame(self):
        """One row per pillar, with a column per delta."""
        pandas = _pandas()
        return pandas.DataFrame(
            [{**{"pillar": label, "date": day}, **smile} for label, day, smile in self.pillars]
        )


def strip_usd_deposits(anchor: date, deposits, **kwargs) -> StrippedDiscountCurve:
    """USD deposit curve: spot plus two government-bond days, Actual/360, log-linear."""
    ql = _quantlib()
    kwargs.setdefault("calendar", ql.UnitedStates(ql.UnitedStates.GovernmentBond))
    kwargs.setdefault("settlement_days", 2)
    kwargs.setdefault("deposit_day_counter", ql.Actual360())
    kwargs.setdefault("convention", ql.ModifiedFollowing)
    kwargs.setdefault("end_of_month", False)
    kwargs.setdefault("discount_day_counter", ql.Actual365Fixed())
    return strip_discount_curve(anchor, deposits=deposits, **kwargs)


def strip_discount_curve(
    anchor: date,
    deposits=(),
    swaps=(),
    *,
    calendar=None,
    settlement_days: int = 2,
    deposit_day_counter=None,
    fixed_day_counter=None,
    fixed_frequency=None,
    convention=None,
    end_of_month: bool = False,
    discount_day_counter=None,
) -> StrippedDiscountCurve:
    """Bootstrap a log-linear discount curve from deposits and fixed-vs-ibor swaps.

    ``deposits`` and ``swaps`` are ``(tenor, rate)`` pairs or a DataFrame with
    those columns. Rates are decimals. The swap fixed leg defaults to annual
    Actual/360 against 3-month USD Libor on the same curve.
    """
    ql = _quantlib()
    if calendar is None:
        calendar = ql.UnitedStates(ql.UnitedStates.GovernmentBond)
    deposit_rows = _tenor_rate(deposits)
    swap_rows = _tenor_rate(swaps)
    if not deposit_rows and not swap_rows:
        raise ValueError("a discount curve needs a deposit or a swap")
    if deposit_day_counter is None:
        deposit_day_counter = ql.Actual360()
    if fixed_day_counter is None:
        fixed_day_counter = ql.Actual360()
    if fixed_frequency is None:
        fixed_frequency = ql.Annual
    if convention is None:
        convention = ql.ModifiedFollowing
    if discount_day_counter is None:
        discount_day_counter = ql.Actual365Fixed()

    settings = ql.Settings.instance()
    previous = settings.evaluationDate
    settings.evaluationDate = _ql_date(ql, anchor)
    try:
        helpers = [
            ql.DepositRateHelper(
                float(rate),
                _ql_period(ql, tenor),
                settlement_days,
                calendar,
                convention,
                end_of_month,
                deposit_day_counter,
            )
            for tenor, rate in deposit_rows
        ]
        handle = None
        if swap_rows:
            handle = ql.RelinkableYieldTermStructureHandle()
            ibor = ql.USDLibor(ql.Period(3, ql.Months), handle)
            helpers.extend(
                ql.SwapRateHelper(
                    float(rate),
                    _ql_period(ql, tenor),
                    calendar,
                    fixed_frequency,
                    convention,
                    fixed_day_counter,
                    ibor,
                )
                for tenor, rate in swap_rows
            )
        curve = ql.PiecewiseLogLinearDiscount(_ql_date(ql, anchor), helpers, discount_day_counter)
        curve.enableExtrapolation()
        if handle is not None:
            handle.linkTo(curve)
        pillars = tuple(
            (_py_date(helper.pillarDate()), float(curve.discount(helper.pillarDate())))
            for helper in helpers
        )
    finally:
        settings.evaluationDate = previous
    return StrippedDiscountCurve(anchor=anchor, pillars=pillars, _curve=curve)


def strip_gofo_to_lease(
    anchor: date,
    spot: float,
    gofo_quotes,
    discount: StrippedDiscountCurve,
    spec: QuoteSpec,
) -> StrippedLease:
    """Turn GOFO quotes into the lease rates the catalogue stores.

    GOFO is ``forward = spot * (1 + GOFO * tau)`` with ``tau`` Actual/360 from
    the gold spot date. The lease rate is the rate that makes the library
    outright equal that forward. The returned discount pillars include the
    gold spot date and every GOFO value date.
    """
    if spot <= 0.0:
        raise ValueError("spot must be positive")
    spot_day = spot_date(spec, anchor)
    london = spec_calendar(spec)
    rows = []
    for tenor, rate in _tenor_rate(gofo_quotes):
        value_date = advance_tenor(london, spot_day, tenor)
        tau = (value_date - spot_day).days / 360.0
        if tau <= 0.0:
            raise ValueError(f"{tenor} value date {value_date} is not after spot {spot_day}")
        forward = spot * (1.0 + rate * tau)
        growth = 1.0 + rate * tau
        lease = (discount.df(spot_day) / discount.df(value_date) / growth - 1.0) / tau
        rows.append(
            {
                "tenor": tenor,
                "value_date": value_date,
                "gofo": rate,
                "lease": lease,
                "tau": tau,
                "forward": forward,
            }
        )
    stored = tuple(rows)
    value_dates = [cast(date, row["value_date"]) for row in stored]
    return StrippedLease(
        spot_date=spot_day,
        rows=stored,
        lease_pillars=tuple(
            (day, cast(float, row["lease"])) for day, row in zip(value_dates, stored, strict=True)
        ),
        discount_pillars=discount.pillars_for([spot_day, *value_dates]),
    )


def delta_pillars_from_rr_bf(
    anchor: date,
    vol_quotes,
    spec: QuoteSpec,
    include_anchor: bool = True,
) -> StrippedSmile:
    """Convert broker smiles to the put-delta grid the library stores.

    Each quote is an at-the-money vol, a 25-delta risk reversal and butterfly,
    and optionally the 10-delta pair. A negative reversal means the put is
    richer. The anchor uses the first smile, so an expiry before the first
    pillar is still inside the grid.
    """
    parsed = _vol_rows(vol_quotes)
    if not parsed:
        raise ValueError("a smile needs a tenor")
    has_wings = all("rr10" in row for row in parsed)
    if any(("rr10" in row) != has_wings for row in parsed):
        raise ValueError("pass the 10-delta reversal and butterfly on every tenor, or on none")
    spot_day = spot_date(spec, anchor)
    london = spec_calendar(spec)
    pillars: list[tuple[str, date, dict[float, float]]] = []
    for row in parsed:
        smile = _smile(row, has_wings)
        pillars.append((row["tenor"], advance_tenor(london, spot_day, row["tenor"]), smile))
    if include_anchor:
        pillars.insert(0, ("anchor", anchor, dict(pillars[0][2])))
    unique: list[tuple[str, date, dict[float, float]]] = []
    seen = set()
    for label, day, smile in pillars:
        if day in seen:
            continue
        seen.add(day)
        unique.append((label, day, smile))
    deltas = (10.0, 25.0, 50.0, 75.0, 90.0) if has_wings else (25.0, 50.0, 75.0)
    return StrippedSmile(pillars=tuple(unique), deltas=deltas)


def spec_calendar(spec: QuoteSpec):
    """The calendar a point quote uses for spot and expiry."""
    from sjd.markets.rates import calendar
    from sjd.markets.specs import PointDates

    if not isinstance(spec.dates, PointDates):
        raise ValueError(f"{spec.market} {spec.currency} is not a point quote")
    return calendar(spec.dates.calendar)


def _smile(row: Mapping[str, float], wings: bool) -> dict[float, float]:
    put25, call25 = wing_vols(row["atm"], row["rr25"], row["bf25"])
    smile = {25.0: put25, 50.0: row["atm"], 75.0: call25}
    if wings:
        put10, call10 = wing_vols(row["atm"], row["rr10"], row["bf10"])
        smile[10.0] = put10
        smile[90.0] = call10
    if any(vol <= 0.0 for vol in smile.values()):
        raise ValueError(f"{row['tenor']} smile has a volatility that is not positive")
    return smile


def _tenor_rate(quotes) -> list[tuple[str, float]]:
    if quotes is None:
        return []
    if hasattr(quotes, "columns"):
        return [(str(row.tenor), float(row.rate)) for row in quotes.itertuples(index=False)]
    if isinstance(quotes, Mapping):
        return [(str(tenor), float(rate)) for tenor, rate in quotes.items()]
    return [(str(tenor), float(rate)) for tenor, rate in quotes]


def _vol_rows(quotes) -> list[dict]:
    if hasattr(quotes, "columns"):
        columns = set(quotes.columns)
        rows = []
        for row in quotes.itertuples(index=False):
            item = {
                "tenor": str(row.tenor),
                "atm": float(row.atm),
                "rr25": float(row.rr25),
                "bf25": float(row.bf25),
            }
            if "rr10" in columns:
                item["rr10"] = float(row.rr10)
                item["bf10"] = float(row.bf10)
            rows.append(item)
        return rows
    rows = []
    for record in quotes:
        if len(record) == 4:
            tenor, atm, rr25, bf25 = record
            rows.append(
                {"tenor": str(tenor), "atm": float(atm), "rr25": float(rr25), "bf25": float(bf25)}
            )
        elif len(record) == 6:
            tenor, atm, rr25, bf25, rr10, bf10 = record
            rows.append(
                {
                    "tenor": str(tenor),
                    "atm": float(atm),
                    "rr25": float(rr25),
                    "bf25": float(bf25),
                    "rr10": float(rr10),
                    "bf10": float(bf10),
                }
            )
        else:
            raise ValueError("a smile row is tenor, atm, rr25, bf25, and optionally rr10, bf10")
    return rows


def _ql_period(ql, label: str):
    count, unit = parse_tenor(label)
    ql_unit = {
        sjd.TimeUnit.DAYS: ql.Days,
        sjd.TimeUnit.WEEKS: ql.Weeks,
        sjd.TimeUnit.MONTHS: ql.Months,
        sjd.TimeUnit.YEARS: ql.Years,
    }[unit]
    return ql.Period(count, ql_unit)


def _ql_date(ql, day: date):
    return ql.Date(day.day, day.month, day.year)


def _py_date(ql_date) -> date:
    return date(ql_date.year(), ql_date.month(), ql_date.dayOfMonth())


def _quantlib():
    try:
        import QuantLib as ql
    except ImportError as error:
        raise ImportError(
            "QuantLib is required to strip discount curves; install the sjd rates extra"
        ) from error
    return ql


def _pandas():
    try:
        import pandas
    except ImportError as error:
        raise ImportError("pandas is required for this table; install the sjd front extra") from error
    return pandas
