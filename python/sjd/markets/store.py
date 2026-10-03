"""Read stored pillars for one anchor date."""

import sqlite3
from datetime import date


class PillarNotFound(LookupError):
    """The database has no pillars for this anchor, market, and currency."""


def _as_iso(day: date) -> str:
    return day.isoformat()


def _as_date(value: str) -> date:
    return date.fromisoformat(value)


def spot(connection: sqlite3.Connection, anchor: date, market: str, currency: str) -> float:
    row = connection.execute(
        """
        SELECT value FROM spot
        WHERE anchor_date = ? AND market = ? AND currency = ?
        """,
        (_as_iso(anchor), market, currency),
    ).fetchone()
    if row is None:
        raise PillarNotFound(f"No {market} {currency} spot on {anchor.isoformat()}.")
    return float(row[0])


def forward_pillars(
    connection: sqlite3.Connection, anchor: date, market: str, currency: str
) -> list[tuple[date, float]]:
    rows = connection.execute(
        """
        SELECT pillar_date, value FROM forward_pillar
        WHERE anchor_date = ? AND market = ? AND currency = ?
        ORDER BY pillar_date
        """,
        (_as_iso(anchor), market, currency),
    ).fetchall()
    if not rows:
        raise PillarNotFound(
            f"No {market} {currency} forward pillars on {anchor.isoformat()}."
        )
    return [(_as_date(pillar), float(value)) for pillar, value in rows]


def discount_pillars(
    connection: sqlite3.Connection, anchor: date, currency: str
) -> list[tuple[date, float]]:
    rows = connection.execute(
        """
        SELECT pillar_date, discount_factor FROM discount_pillar
        WHERE anchor_date = ? AND currency = ?
        ORDER BY pillar_date
        """,
        (_as_iso(anchor), currency),
    ).fetchall()
    if not rows:
        raise PillarNotFound(f"No {currency} discount pillars on {anchor.isoformat()}.")
    return [(_as_date(pillar), float(factor)) for pillar, factor in rows]


def volatility_grid(
    connection: sqlite3.Connection, anchor: date, market: str, currency: str
) -> tuple[list[date], list[float], list[list[float]]]:
    """Return observation dates, strike dimension, and volatility by strike then date."""
    rows = connection.execute(
        """
        SELECT pillar_date, strike_dimension, volatility FROM volatility_pillar
        WHERE anchor_date = ? AND market = ? AND currency = ?
        ORDER BY pillar_date, strike_dimension
        """,
        (_as_iso(anchor), market, currency),
    ).fetchall()
    if not rows:
        raise PillarNotFound(
            f"No {market} {currency} volatility pillars on {anchor.isoformat()}."
        )
    by_date: dict[date, dict[float, float]] = {}
    for pillar, strike, vol in rows:
        by_date.setdefault(_as_date(pillar), {})[float(strike)] = float(vol)
    dates = sorted(by_date)
    strikes = sorted(by_date[dates[0]])
    for day in dates[1:]:
        if sorted(by_date[day]) != strikes:
            raise PillarNotFound(
                f"{market} {currency} volatility on {day.isoformat()} "
                "does not use the same strike dimension as the first pillar."
            )
    volatility = [[by_date[day][strike] for day in dates] for strike in strikes]
    return dates, strikes, volatility
