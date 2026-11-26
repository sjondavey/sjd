"""Schedules, multi-period structures, and a deterministic present-value solve.

A structure strip keeps one group of legs per settlement. Forwards have one
leg, collars and participating forwards have two, and seagulls have three.
The present value is the bank's mark to market.
"""

from __future__ import annotations

import logging
from calendar import monthrange
from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from datetime import date, timedelta
from numbers import Real
from typing import Any

import sjd
from sjd.markets.rates import LoadedMarket
from sjd.markets.specs import PointDates
from sjd.strategy import Client, _put_call_sides

logger = logging.getLogger(__name__)


@dataclass(frozen=True)
class Period:
    """One settlement on a schedule."""

    settlement: date
    expiry: date | None = None
    averaging_dates: tuple[date, ...] = ()
    index: int = 0


@dataclass(frozen=True)
class Schedule:
    """Settlements, and the expiry or averaging dates that go with them."""

    periods: tuple[Period, ...]

    def __len__(self) -> int:
        return len(self.periods)

    def __post_init__(self) -> None:
        if not self.periods:
            raise ValueError("a schedule needs a period")
        for index in range(len(self.periods) - 1):
            if self.periods[index + 1].settlement <= self.periods[index].settlement:
                raise ValueError("settlements must be strictly increasing")

    @classmethod
    def monthly(cls, market: LoadedMarket, start: date, months: int, convention=None) -> Schedule:
        """One period a month from ``start``.

        A point quote adjusts ``start`` and then reads the option expiry off
        the settlement. A calendar-month quote uses the contract month that
        contains each unadjusted date.
        """
        if months <= 0:
            raise ValueError("months must be positive")
        if convention is None:
            convention = sjd.BusinessDayConvention.MODIFIED_FOLLOWING
        holidays = market.calendar()
        periods = []
        for index in range(months):
            raw = _add_months(start, index)
            if isinstance(market.spec.dates, PointDates):
                settlement = holidays.adjust(raw, convention)
                periods.append(
                    Period(settlement=settlement, expiry=market.option_expiry(settlement), index=index)
                )
            else:
                periods.append(
                    Period(
                        settlement=market.month_settlement(raw),
                        averaging_dates=tuple(market.pricing_dates(raw)),
                        index=index,
                    )
                )
        return cls(tuple(periods))

    @classmethod
    def from_tenors(cls, market: LoadedMarket, start: date, tenors: Sequence[str], convention=None) -> Schedule:
        """One period per tenor, advanced from ``start``."""
        from sjd.markets.strip import advance_tenor

        if convention is None:
            convention = sjd.BusinessDayConvention.MODIFIED_FOLLOWING
        holidays = market.calendar()
        periods = []
        for index, tenor in enumerate(tenors):
            settlement = advance_tenor(holidays, start, tenor, convention)
            if isinstance(market.spec.dates, PointDates):
                periods.append(
                    Period(settlement=settlement, expiry=market.option_expiry(settlement), index=index)
                )
            else:
                periods.append(
                    Period(
                        settlement=market.month_settlement(settlement),
                        averaging_dates=tuple(market.pricing_dates(settlement)),
                        index=index,
                    )
                )
        return cls(tuple(periods))


def step_ladder(start: float, step: float, count: int, step_every: int = 12) -> list[float]:
    """Repeat ``start`` for ``step_every`` periods, then add ``step``, and so on."""
    if count <= 0:
        raise ValueError("count must be positive")
    if step_every <= 0:
        raise ValueError("step_every must be positive")
    return [start + (index // step_every) * step for index in range(count)]


@dataclass(frozen=True)
class PeriodLegs:
    """The legs of one period, and the strikes that describe them."""

    period: Period
    contracts: tuple
    strikes: tuple[tuple[str, float], ...]

    def strike(self, name: str) -> float:
        return dict(self.strikes)[name]

    def pv(self, rates_environment) -> float:
        return sum(contract.get_value(rates_environment) for contract in self.contracts)


@dataclass(frozen=True)
class StructureStrip:
    """A strip of the same strategy on every period of a schedule."""

    kind: str
    periods: tuple[PeriodLegs, ...]

    def __post_init__(self) -> None:
        if not self.periods:
            raise ValueError("a strip needs a period")

    def contracts(self) -> list:
        """Every leg, in period order."""
        instruments: list[Any] = []
        for group in self.periods:
            instruments.extend(group.contracts)
        return instruments

    def portfolio(self, margin=None):
        """A portfolio of the strip, optionally with a CSA."""
        return self._portfolio(margin, log=True)

    def _portfolio(self, margin=None, log: bool = False):
        contracts = self.contracts()
        if log:
            logger.info(
                "built %s strip periods=%s contracts=%s", self.kind, len(self.periods), len(contracts)
            )
        if margin is None:
            return sjd.Portfolio(contracts)
        return sjd.Portfolio(contracts, margin)

    def pv(self, market) -> float:
        """Present value of the bank's book."""
        environment = _environment(market)
        portfolio = self._portfolio()
        sjd.require_valid(portfolio, environment)
        return portfolio.get_value(environment)

    def period_pvs(self, market) -> list[float]:
        """Present value of each period."""
        environment = _environment(market)
        portfolio = self._portfolio()
        sjd.require_valid(portfolio, environment)
        return [group.pv(environment) for group in self.periods]

    def payoff(self, spots: Sequence[float]) -> list[float]:
        """Undiscounted settlement value if every fixing prints ``spot``."""
        contracts = self.contracts()
        return [sum(contract.get_payoff(spot) for contract in contracts) for spot in spots]

    def schedule_table(self, market: LoadedMarket, group_every: int = 12):
        """One row per settlement: outright, discount factor, strikes, and value."""
        if group_every <= 0:
            raise ValueError("group_every must be positive")
        pandas = _pandas()
        environment = market.environment
        portfolio = self._portfolio()
        sjd.require_valid(portfolio, environment)
        rows = []
        for index, group in enumerate(self.periods):
            row = {
                "date": group.period.settlement,
                "group": index // group_every + 1,
                "outright": market.forward.get_forward(group.period.settlement),
                "discount_factor": market.discount.get_discount_factor(group.period.settlement),
            }
            row.update(dict(group.strikes))
            row["pv"] = group.pv(environment)
            rows.append(row)
        return pandas.DataFrame(rows)

    def group_summary(self, market: LoadedMarket, group_every: int = 12):
        """One row per group of periods, with the present value and the first strikes."""
        table = self.schedule_table(market, group_every)
        pandas = _pandas()
        rows = []
        for group, frame in table.groupby("group", sort=True):
            row = {"group": group, "first": frame["date"].iloc[0], "last": frame["date"].iloc[-1]}
            for name, _value in self.periods[0].strikes:
                row[name] = frame[name].iloc[0]
            row["pv"] = frame["pv"].sum()
            rows.append(row)
        return pandas.DataFrame(rows)


@dataclass(frozen=True)
class SolvedStructure:
    """The parameter that makes a structure worth ``pv``."""

    parameter: float
    structure: StructureStrip
    pv: float


class ForwardStrip:
    """One forward on each settlement. The bank's side is ``buy_sell``."""

    @staticmethod
    def build(schedule: Schedule, strikes, volume, buy_sell=sjd.BuySell.BUY) -> StructureStrip:
        count = len(schedule.periods)
        levels = _broadcast(strikes, count, "strikes")
        volumes = _broadcast(volume, count, "volume")
        periods = []
        for period, strike, size in zip(schedule.periods, levels, volumes, strict=True):
            if strike <= 0.0 or size <= 0.0:
                raise ValueError("strikes and volume must be positive")
            contract = _forward_contract(period, strike, size, buy_sell)
            periods.append(PeriodLegs(period, (contract,), (("strike", strike),)))
        return StructureStrip("forward", tuple(periods))

    @staticmethod
    def from_moneyness(
        schedule: Schedule,
        market: LoadedMarket,
        moneyness: float,
        volume,
        buy_sell=sjd.BuySell.BUY,
        strike_decimals: int = 2,
    ) -> StructureStrip:
        """Strike each period at its forward times ``moneyness``."""
        if moneyness <= 0.0:
            raise ValueError("moneyness must be positive")
        strikes = [
            round(_period_forward(market, period) * moneyness, strike_decimals)
            for period in schedule.periods
        ]
        return ForwardStrip.build(schedule, strikes, volume, buy_sell)


class CollarStrip:
    """A collar on each settlement. ``client`` sets the bank's side."""

    @staticmethod
    def build(
        schedule: Schedule,
        put_strikes,
        call_strikes=None,
        *,
        width: float | None = None,
        volume,
        client: Client = Client.PRODUCER,
    ) -> StructureStrip:
        count = len(schedule.periods)
        puts, calls = _wings(count, put_strikes, call_strikes, width, "put_strikes", "call_strikes")
        volumes = _broadcast(volume, count, "volume")
        put_side, call_side = _put_call_sides(client)
        periods = []
        for period, put, call, size in zip(schedule.periods, puts, calls, volumes, strict=True):
            _require_spread(put, call, size)
            contracts = (
                _option_contract(period, put, sjd.PutCall.PUT, size, put_side),
                _option_contract(period, call, sjd.PutCall.CALL, size, call_side),
            )
            periods.append(PeriodLegs(period, contracts, (("put", put), ("call", call))))
        return StructureStrip("collar", tuple(periods))

    @staticmethod
    def from_moneyness(
        schedule: Schedule,
        market: LoadedMarket,
        put_moneyness: float,
        call_moneyness: float | None = None,
        *,
        width: float | None = None,
        volume,
        client: Client = Client.PRODUCER,
        strike_decimals: int = 2,
    ) -> StructureStrip:
        """Strikes are the period forward times moneyness. ``width`` is added to the put."""
        puts, calls = _moneyness_wings(
            schedule, market, put_moneyness, call_moneyness, width, strike_decimals
        )
        return CollarStrip.build(schedule, puts, calls, volume=volume, client=client)


class SeagullStrip:
    """A collar plus a lower put on each settlement."""

    @staticmethod
    def build(
        schedule: Schedule,
        floor_strikes,
        put_strikes,
        call_strikes,
        volume,
        client: Client = Client.PRODUCER,
    ) -> StructureStrip:
        count = len(schedule.periods)
        floors = _broadcast(floor_strikes, count, "floor_strikes")
        puts = _broadcast(put_strikes, count, "put_strikes")
        calls = _broadcast(call_strikes, count, "call_strikes")
        volumes = _broadcast(volume, count, "volume")
        put_side, call_side = _put_call_sides(client)
        floor_side = sjd.BuySell.BUY if put_side == sjd.BuySell.SELL else sjd.BuySell.SELL
        periods = []
        for period, floor, put, call, size in zip(
            schedule.periods, floors, puts, calls, volumes, strict=True
        ):
            if min(floor, put, call, size) <= 0.0:
                raise ValueError("strikes and volume must be positive")
            if not floor < put < call:
                raise ValueError("need floor_strike < put_strike < call_strike")
            contracts = (
                _option_contract(period, put, sjd.PutCall.PUT, size, put_side),
                _option_contract(period, call, sjd.PutCall.CALL, size, call_side),
                _option_contract(period, floor, sjd.PutCall.PUT, size, floor_side),
            )
            periods.append(
                PeriodLegs(period, contracts, (("floor", floor), ("put", put), ("call", call)))
            )
        return StructureStrip("seagull", tuple(periods))

    @staticmethod
    def from_moneyness(
        schedule: Schedule,
        market: LoadedMarket,
        floor_moneyness: float,
        put_moneyness: float,
        call_moneyness: float,
        volume,
        client: Client = Client.PRODUCER,
        strike_decimals: int = 2,
    ) -> StructureStrip:
        if not 0.0 < floor_moneyness < put_moneyness < call_moneyness:
            raise ValueError("need 0 < floor_moneyness < put_moneyness < call_moneyness")
        floors, puts, calls = [], [], []
        for period in schedule.periods:
            level = _period_forward(market, period)
            floors.append(round(level * floor_moneyness, strike_decimals))
            puts.append(round(level * put_moneyness, strike_decimals))
            calls.append(round(level * call_moneyness, strike_decimals))
        return SeagullStrip.build(schedule, floors, puts, calls, volume, client)


class ParticipatingForwardStrip:
    """A put on the full volume and a call on the volume the client gives up."""

    @staticmethod
    def build(
        schedule: Schedule,
        put_strikes,
        call_strikes,
        volume,
        participation,
        client: Client = Client.PRODUCER,
    ) -> StructureStrip:
        count = len(schedule.periods)
        puts = _broadcast(put_strikes, count, "put_strikes")
        calls = _broadcast(call_strikes, count, "call_strikes")
        volumes = _broadcast(volume, count, "volume")
        shares = _broadcast(participation, count, "participation")
        put_side, call_side = _put_call_sides(client)
        periods = []
        for period, put, call, size, share in zip(
            schedule.periods, puts, calls, volumes, shares, strict=True
        ):
            _require_spread(put, call, size)
            if not 0.0 < share < 1.0:
                raise ValueError("participation must be between 0 and 1")
            contracts = (
                _option_contract(period, put, sjd.PutCall.PUT, size, put_side),
                _option_contract(period, call, sjd.PutCall.CALL, size * (1.0 - share), call_side),
            )
            periods.append(PeriodLegs(period, contracts, (("put", put), ("call", call))))
        return StructureStrip("participating_forward", tuple(periods))

    @staticmethod
    def from_moneyness(
        schedule: Schedule,
        market: LoadedMarket,
        put_moneyness: float,
        call_moneyness: float,
        volume,
        participation,
        client: Client = Client.PRODUCER,
        strike_decimals: int = 2,
    ) -> StructureStrip:
        puts, calls = _moneyness_wings(
            schedule, market, put_moneyness, call_moneyness, None, strike_decimals
        )
        return ParticipatingForwardStrip.build(
            schedule, puts, calls, volume, participation, client
        )


def solve_pv(
    build,
    market,
    target_pv: float = 0.0,
    bracket: tuple[float, float] = (0.01, 10_000.0),
    xtol: float = 1e-6,
) -> SolvedStructure:
    """Find the parameter that makes ``build(parameter)`` worth ``target_pv``.

    ``build`` is checked at the two ends of ``bracket``. The search then marks
    each trial to the same curves. A higher strike is worth less to the bank
    when the bank is buying, and the bracket has to contain the target either way.
    """
    try:
        from scipy.optimize import brentq
    except ImportError as error:
        raise ImportError("scipy is required for solve_pv; install the sjd front extra") from error
    low, high = bracket
    if not low < high:
        raise ValueError("bracket low must be below high")
    environment = _environment(market)
    at_low_structure = build(low)
    at_high_structure = build(high)
    sjd.require_valid(at_low_structure._portfolio(), environment)
    sjd.require_valid(at_high_structure._portfolio(), environment)
    at_low = at_low_structure._portfolio().get_value(environment)
    at_high = at_high_structure._portfolio().get_value(environment)
    if not min(at_low, at_high) <= target_pv <= max(at_low, at_high):
        raise ValueError(
            f"target {target_pv:,.2f} is outside {at_high:,.2f} at {high:,.2f} "
            f"and {at_low:,.2f} at {low:,.2f}"
        )

    def objective(parameter: float) -> float:
        return build(parameter)._portfolio().get_value(environment) - target_pv

    parameter = float(brentq(objective, low, high, xtol=xtol))
    structure = build(parameter)
    pv = structure._portfolio().get_value(environment)
    return SolvedStructure(parameter=parameter, structure=structure, pv=pv)


def compare_structures(
    structures: Mapping[str, StructureStrip], market: LoadedMarket, group_every: int = 12
):
    """A group summary and a settlement table for several named structures.

    Returns ``(summary, schedule)``. The summary sums present value by group.
    The schedule has the outright, each structure's strikes, and each value.
    """
    if not structures:
        raise ValueError("compare_structures needs a structure")
    schedule = None
    for name, structure in structures.items():
        table = structure.schedule_table(market, group_every)
        strike_columns = [
            column
            for column in table.columns
            if column not in {"date", "group", "outright", "discount_factor", "pv"}
        ]
        renamed = table[["date", *strike_columns, "pv"]].rename(
            columns={column: f"{name}_{column}" for column in (*strike_columns, "pv")}
        )
        if schedule is None:
            schedule = table[["date", "group", "outright", "discount_factor"]].merge(renamed, on="date")
        else:
            schedule = schedule.merge(renamed, on="date")
    if schedule is None:
        raise ValueError("compare_structures needs a structure")
    value_columns = [f"{name}_pv" for name in structures]
    summary = schedule.groupby("group", as_index=False)[value_columns].sum()
    return summary, schedule


def plot_structures(structures: Mapping[str, StructureStrip], market: LoadedMarket, axes=None):
    """Outright and strikes on the left, and the combined payoff on the right."""
    try:
        import matplotlib.pyplot as plt
    except ImportError as error:
        raise ImportError("matplotlib is required to plot structures") from error
    _summary, schedule = compare_structures(structures, market)
    if axes is None:
        _figure, axes = plt.subplots(1, 2, figsize=(11, 4))
    dates = list(schedule["date"])
    grid = []
    day = dates[0]
    while day <= dates[-1]:
        grid.append(day)
        day += timedelta(days=7)
    axes[0].plot(grid, [market.forward.get_forward(day) for day in grid], label="outright")
    for name, structure in structures.items():
        for key, _value in structure.periods[0].strikes:
            label = name if key == "strike" else f"{name} {key}"
            axes[0].plot(
                dates,
                [group.strike(key) for group in structure.periods],
                drawstyle="steps-post",
                label=label,
            )
    axes[0].set_title("Outright and strikes")
    axes[0].set_ylabel("price")
    axes[0].legend()

    strike_columns = [
        column
        for column in schedule.columns
        if column not in {"date", "group", "outright", "discount_factor"} and not column.endswith("_pv")
    ]
    low = min(schedule["outright"].min(), *(schedule[column].min() for column in strike_columns)) * 0.85
    high = max(schedule["outright"].max(), *(schedule[column].max() for column in strike_columns)) * 1.15
    spots = [low + (high - low) * index / 200 for index in range(201)]
    for name, structure in structures.items():
        axes[1].plot(spots, structure.payoff(spots), label=name)
    axes[1].axhline(0.0, color="black", linewidth=0.6)
    axes[1].set_title("Payoff if every fixing prints the same spot")
    axes[1].set_xlabel("spot")
    axes[1].set_ylabel("value")
    axes[1].legend()
    figure = axes[0].figure
    figure.tight_layout()
    return figure


def _environment(market):
    if isinstance(market, LoadedMarket):
        return market.environment
    if isinstance(market, dict):
        return market["environment"]
    return market


def _period_forward(market: LoadedMarket, period: Period) -> float:
    if period.averaging_dates:
        return sum(market.forward.get_forward(day) for day in period.averaging_dates) / len(
            period.averaging_dates
        )
    return market.forward.get_forward(period.settlement)


def _forward_contract(period: Period, strike: float, volume: float, buy_sell):
    if period.averaging_dates:
        return sjd.AsianForward(period.averaging_dates, period.settlement, strike, volume, buy_sell)
    return sjd.EuropeanForward(period.settlement, strike, volume, buy_sell)


def _option_contract(period: Period, strike: float, option_type, volume: float, buy_sell):
    if period.averaging_dates:
        return sjd.AsianOption(
            period.averaging_dates,
            period.settlement,
            strike,
            option_type,
            volume,
            period.settlement,
            0.0,
            buy_sell,
        )
    if period.expiry is None:
        raise ValueError(f"{period.settlement} has no option expiry")
    return sjd.EuropeanOption(
        period.expiry,
        period.settlement,
        strike,
        option_type,
        volume,
        period.settlement,
        0.0,
        buy_sell,
    )


def _broadcast(value, count: int, label: str) -> list[float]:
    if isinstance(value, bool):
        raise TypeError(f"{label} must be a number or a sequence of numbers")
    if isinstance(value, Real):
        return [float(value)] * count
    values = [float(item) for item in value]
    if len(values) != count:
        raise ValueError(f"{label} must be one number or {count} numbers")
    return values


def _wings(count, left, right, width, left_name, right_name):
    lows = _broadcast(left, count, left_name)
    if (right is None) == (width is None):
        raise ValueError(f"pass {right_name} or width, not both")
    if right is None:
        highs = [level + float(width) for level in lows]
    else:
        highs = _broadcast(right, count, right_name)
    return lows, highs


def _moneyness_wings(schedule, market, put_moneyness, call_moneyness, width, strike_decimals):
    if put_moneyness <= 0.0:
        raise ValueError("put moneyness must be positive")
    if (call_moneyness is None) == (width is None):
        raise ValueError("pass call_moneyness or width, not both")
    if call_moneyness is not None and call_moneyness <= put_moneyness:
        raise ValueError("call moneyness must be above put moneyness")
    puts, calls = [], []
    for period in schedule.periods:
        level = _period_forward(market, period)
        put = round(level * put_moneyness, strike_decimals)
        call = round(level * call_moneyness, strike_decimals) if call_moneyness is not None else put + width
        puts.append(put)
        calls.append(call)
    return puts, calls


def _require_spread(put: float, call: float, volume: float) -> None:
    if put <= 0.0 or call <= 0.0 or volume <= 0.0:
        raise ValueError("strikes and volume must be positive")
    if put >= call:
        raise ValueError("put strike must be below call strike")


def _add_months(day: date, count: int) -> date:
    month_index = day.month - 1 + count
    year = day.year + month_index // 12
    month = month_index % 12 + 1
    last = monthrange(year, month)[1]
    return date(year, month, min(day.day, last))


def _pandas():
    try:
        import pandas
    except ImportError as error:
        raise ImportError("pandas is required for structure tables; install the sjd front extra") from error
    return pandas
