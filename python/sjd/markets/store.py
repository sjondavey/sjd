"""Read and write stored pillars for one rates label."""

import sqlite3
from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from datetime import date, datetime, timezone


class PillarNotFound(LookupError):
    """The database has no pillars for this label, market, and currency."""


class RatesLabelError(ValueError):
    """A rates label cannot be saved because its anchor dates do not agree."""


@dataclass(frozen=True)
class RatesLabel:
    """One named snapshot of a rates environment.

    ``name`` and ``as_of`` identify it. ``as_of`` is stored to the second.
    ``anchor`` is the curve date, and every instrument on the label uses it.
    """

    name: str
    as_of: datetime
    anchor: date

    @property
    def as_of_text(self) -> str:
        """The datetime as it is stored, ``YYYY-MM-DDTHH:MM:SS``."""
        return self.as_of.strftime("%Y-%m-%dT%H:%M:%S")


def as_of_text(value: datetime | str) -> str:
    """Canonical second-resolution text for a label datetime.

    An aware datetime is stored in UTC. A string is an ISO-8601 datetime.
    """
    if isinstance(value, str):
        try:
            parsed = datetime.fromisoformat(value)
        except ValueError as error:
            raise ValueError(f"as_of must be an ISO-8601 datetime, got {value!r}") from error
    elif isinstance(value, datetime):
        parsed = value
    else:
        raise TypeError(f"as_of must be a datetime or an ISO-8601 string, got {value!r}")
    if parsed.tzinfo is not None:
        parsed = parsed.astimezone(timezone.utc).replace(tzinfo=None)
    return parsed.strftime("%Y-%m-%dT%H:%M:%S")


def _label_name(name: str) -> str:
    if not isinstance(name, str) or name.strip() == "" or name != name.strip():
        raise ValueError(f"a rates label name must be a non-empty string, got {name!r}")
    return name


def _label_from_row(row) -> RatesLabel:
    return RatesLabel(name=row[0], as_of=datetime.fromisoformat(row[1]), anchor=date.fromisoformat(row[2]))


def rates_labels(connection: sqlite3.Connection, name: str | None = None) -> list[RatesLabel]:
    """Every label, or every instance of ``name``, oldest first."""
    if name is None:
        rows = connection.execute(
            "SELECT name, as_of, anchor_date FROM rates_label ORDER BY name, as_of"
        ).fetchall()
    else:
        rows = connection.execute(
            """
            SELECT name, as_of, anchor_date FROM rates_label
            WHERE name = ? ORDER BY as_of
            """,
            (_label_name(name),),
        ).fetchall()
    return [_label_from_row(row) for row in rows]


def rates_label(
    connection: sqlite3.Connection, name: str, as_of: datetime | str | None = None
) -> RatesLabel:
    """The named label. Omit ``as_of`` for the newest instance."""
    key = _label_name(name)
    if as_of is None:
        rows = connection.execute(
            """
            SELECT name, as_of, anchor_date FROM rates_label
            WHERE name = ? ORDER BY as_of DESC LIMIT 1
            """,
            (key,),
        ).fetchall()
        if not rows:
            raise PillarNotFound(f"No rates label named {key!r}.")
        return _label_from_row(rows[0])
    stamp = as_of_text(as_of)
    row = connection.execute(
        """
        SELECT name, as_of, anchor_date FROM rates_label
        WHERE name = ? AND as_of = ?
        """,
        (key, stamp),
    ).fetchone()
    if row is None:
        raise PillarNotFound(f"No rates label {key!r} at {stamp}.")
    return _label_from_row(row)


def ensure_label(
    connection: sqlite3.Connection, name: str, as_of: datetime | str, anchor: date
) -> RatesLabel:
    """Create the label, or return it when it already has this anchor date.

    A second save with a different anchor date is refused. The anchor lives on
    the label, so the pillars under it cannot disagree with each other.
    """
    key = _label_name(name)
    stamp = as_of_text(as_of)
    row = connection.execute(
        "SELECT anchor_date FROM rates_label WHERE name = ? AND as_of = ?",
        (key, stamp),
    ).fetchone()
    if row is None:
        connection.execute(
            "INSERT INTO rates_label (name, as_of, anchor_date) VALUES (?, ?, ?)",
            (key, stamp, anchor.isoformat()),
        )
        return RatesLabel(name=key, as_of=datetime.fromisoformat(stamp), anchor=anchor)
    stored = date.fromisoformat(row[0])
    if stored != anchor:
        raise RatesLabelError(
            f"rates label {key!r} at {stamp} is anchored on {stored.isoformat()}, "
            f"not {anchor.isoformat()}."
        )
    return RatesLabel(name=key, as_of=datetime.fromisoformat(stamp), anchor=stored)


def resolve_label(
    connection: sqlite3.Connection,
    *,
    name: str | None = None,
    as_of: datetime | str | None = None,
    anchor: date | None = None,
) -> RatesLabel:
    """The label a read should use.

    A name selects that label, and the newest instance when ``as_of`` is
    omitted. Without a name, ``anchor`` must belong to exactly one label.
    With neither, the database must hold exactly one label.
    """
    if name is not None:
        found = rates_label(connection, name, as_of)
        if anchor is not None and found.anchor != anchor:
            raise RatesLabelError(
                f"rates label {found.name!r} at {found.as_of_text} is anchored on "
                f"{found.anchor.isoformat()}, not {anchor.isoformat()}."
            )
        return found
    if anchor is not None:
        rows = connection.execute(
            """
            SELECT name, as_of, anchor_date FROM rates_label
            WHERE anchor_date = ? ORDER BY as_of
            """,
            (anchor.isoformat(),),
        ).fetchall()
        if len(rows) == 1:
            return _label_from_row(rows[0])
        if not rows:
            raise PillarNotFound(f"No rates label is anchored on {anchor.isoformat()}.")
        listed = ", ".join(f"{row[0]} at {row[1]}" for row in rows)
        raise PillarNotFound(
            f"anchor {anchor.isoformat()} has labels [{listed}]. Pass label to choose one."
        )
    rows = connection.execute(
        "SELECT name, as_of, anchor_date FROM rates_label ORDER BY as_of DESC"
    ).fetchall()
    if len(rows) == 1:
        return _label_from_row(rows[0])
    if not rows:
        raise PillarNotFound("The database has no rates label.")
    listed = ", ".join(f"{row[0]} at {row[1]}" for row in rows)
    raise PillarNotFound(f"The database has labels [{listed}]. Pass label to choose one.")


def _as_iso(day: date) -> str:
    return day.isoformat()


def _as_date(value: str) -> date:
    return date.fromisoformat(value)


def spot(
    connection: sqlite3.Connection,
    anchor: date,
    market: str,
    currency: str,
    *,
    label: str | None = None,
    as_of: datetime | str | None = None,
) -> float:
    stored = resolve_label(connection, name=label, as_of=as_of, anchor=anchor)
    row = connection.execute(
        """
        SELECT value FROM spot
        WHERE label_name = ? AND as_of = ? AND market = ? AND currency = ?
        """,
        (stored.name, stored.as_of_text, market, currency),
    ).fetchone()
    if row is None:
        raise PillarNotFound(
            f"No {market} {currency} spot on label {stored.name!r} at {stored.as_of_text}."
        )
    return float(row[0])


def forward_pillars(
    connection: sqlite3.Connection,
    anchor: date,
    market: str,
    currency: str,
    *,
    label: str | None = None,
    as_of: datetime | str | None = None,
) -> list[tuple[date, float]]:
    stored = resolve_label(connection, name=label, as_of=as_of, anchor=anchor)
    rows = connection.execute(
        """
        SELECT pillar_date, value FROM forward_pillar
        WHERE label_name = ? AND as_of = ? AND market = ? AND currency = ?
        ORDER BY pillar_date
        """,
        (stored.name, stored.as_of_text, market, currency),
    ).fetchall()
    if not rows:
        raise PillarNotFound(
            f"No {market} {currency} forward pillars on label {stored.name!r} at {stored.as_of_text}."
        )
    return [(_as_date(pillar), float(value)) for pillar, value in rows]


def discount_pillars(
    connection: sqlite3.Connection,
    anchor: date,
    currency: str,
    *,
    label: str | None = None,
    as_of: datetime | str | None = None,
) -> list[tuple[date, float]]:
    stored = resolve_label(connection, name=label, as_of=as_of, anchor=anchor)
    rows = connection.execute(
        """
        SELECT pillar_date, discount_factor FROM discount_pillar
        WHERE label_name = ? AND as_of = ? AND currency = ?
        ORDER BY pillar_date
        """,
        (stored.name, stored.as_of_text, currency),
    ).fetchall()
    if not rows:
        raise PillarNotFound(
            f"No {currency} discount pillars on label {stored.name!r} at {stored.as_of_text}."
        )
    return [(_as_date(pillar), float(factor)) for pillar, factor in rows]


def volatility_grid(
    connection: sqlite3.Connection,
    anchor: date,
    market: str,
    currency: str,
    *,
    label: str | None = None,
    as_of: datetime | str | None = None,
) -> tuple[list[date], list[float], list[list[float]]]:
    """Return observation dates, strike dimension, and volatility by strike then date."""
    stored = resolve_label(connection, name=label, as_of=as_of, anchor=anchor)
    rows = connection.execute(
        """
        SELECT pillar_date, strike_dimension, volatility FROM volatility_pillar
        WHERE label_name = ? AND as_of = ? AND market = ? AND currency = ?
        ORDER BY pillar_date, strike_dimension
        """,
        (stored.name, stored.as_of_text, market, currency),
    ).fetchall()
    if not rows:
        raise PillarNotFound(
            f"No {market} {currency} volatility pillars on label {stored.name!r} at {stored.as_of_text}."
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


def _dated_pairs(pillars: Mapping[date, float] | Sequence[tuple[date, float]]) -> list[tuple[date, float]]:
    if isinstance(pillars, Mapping):
        return [(day, float(value)) for day, value in pillars.items()]
    return [(day, float(value)) for day, value in pillars]


def _label_key(label: RatesLabel) -> tuple[str, str]:
    return label.name, label.as_of_text


def save_spot(
    connection: sqlite3.Connection, label: RatesLabel, market: str, currency: str, value: float
) -> None:
    """Insert or replace the spot for one quote on a label."""
    connection.execute(
        """
        INSERT INTO spot (label_name, as_of, market, currency, value) VALUES (?, ?, ?, ?, ?)
        ON CONFLICT(label_name, as_of, market, currency) DO UPDATE SET value = excluded.value
        """,
        (*_label_key(label), market, currency, float(value)),
    )


def save_discount_pillars(
    connection: sqlite3.Connection,
    label: RatesLabel,
    currency: str,
    pillars: Mapping[date, float] | Sequence[tuple[date, float]],
) -> None:
    """Replace the discount pillars for one currency on a label."""
    connection.execute(
        "DELETE FROM discount_pillar WHERE label_name = ? AND as_of = ? AND currency = ?",
        (*_label_key(label), currency),
    )
    connection.executemany(
        """
        INSERT INTO discount_pillar (label_name, as_of, currency, pillar_date, discount_factor)
        VALUES (?, ?, ?, ?, ?)
        """,
        [(*_label_key(label), currency, _as_iso(day), factor) for day, factor in _dated_pairs(pillars)],
    )


def save_forward_pillars(
    connection: sqlite3.Connection,
    label: RatesLabel,
    market: str,
    currency: str,
    pillars: Mapping[date, float] | Sequence[tuple[date, float]],
) -> None:
    """Replace the forward pillars for one quote on a label."""
    connection.execute(
        """
        DELETE FROM forward_pillar
        WHERE label_name = ? AND as_of = ? AND market = ? AND currency = ?
        """,
        (*_label_key(label), market, currency),
    )
    connection.executemany(
        """
        INSERT INTO forward_pillar (label_name, as_of, market, currency, pillar_date, value)
        VALUES (?, ?, ?, ?, ?, ?)
        """,
        [
            (*_label_key(label), market, currency, _as_iso(day), value)
            for day, value in _dated_pairs(pillars)
        ],
    )


def save_volatility_pillars(
    connection: sqlite3.Connection,
    label: RatesLabel,
    market: str,
    currency: str,
    pillars: Sequence[tuple[date, float, float]],
) -> None:
    """Replace the volatility pillars for one quote on a label.

    Each pillar is a date, a strike dimension, and a volatility.
    """
    connection.execute(
        """
        DELETE FROM volatility_pillar
        WHERE label_name = ? AND as_of = ? AND market = ? AND currency = ?
        """,
        (*_label_key(label), market, currency),
    )
    connection.executemany(
        """
        INSERT INTO volatility_pillar (
            label_name, as_of, market, currency, pillar_date, strike_dimension, volatility
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
        """,
        [
            (*_label_key(label), market, currency, _as_iso(day), float(strike), float(vol))
            for day, strike, vol in pillars
        ],
    )
