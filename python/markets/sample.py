"""One anchor date of dummy pillars for tests.

The catalogue decides what is written. A quote whose forward is stored gets
its forward pillars, a quote that needs a spot gets one, and every quote gets
a surface. A delta surface uses the point dates. A calendar-month surface
uses the quote's forward months. Discount factors are continuous Actual/365
rates, one curve per catalogue currency.

The numbers below are keyed by (market, currency). A catalogue quote with no
sample numbers is an error, so a new market needs an entry here before the
sample can load.
"""

import math
import sqlite3
from datetime import date
from typing import Optional

from markets.schema import initialize
from markets.specs import CATALOGUE, CalendarMonthMoneynessVol, Catalogue, DeltaVol, QuoteSpec

SAMPLE_ANCHOR = date(2026, 9, 16)
XAU_SPOT = 2400.0

USD_CONTINUOUS_RATE = 0.04
ZAR_CONTINUOUS_RATE = 0.08
CONTINUOUS_RATES = {"USD": USD_CONTINUOUS_RATE, "ZAR": ZAR_CONTINUOUS_RATE}

# Added to volatility per year from the anchor, so the surface is not flat.
VOL_TENOR_SLOPE = 0.02

POINT_DATES = (
    date(2026, 9, 16),
    date(2026, 10, 16),
    date(2026, 12, 16),
    date(2027, 3, 16),
    date(2027, 9, 16),
)

# Simple Actual/360 lease rates. No pillar on the anchor: the rate has no tenor there.
GOFO = (
    (date(2026, 10, 16), 0.005),
    (date(2026, 12, 16), 0.008),
    (date(2027, 3, 16), 0.010),
    (date(2027, 9, 16), 0.012),
)

XPT_FORWARD = (
    (date(2026, 9, 16), 1000.0),
    (date(2026, 10, 16), 1004.0),
    (date(2026, 12, 16), 1012.0),
    (date(2027, 3, 16), 1024.0),
    (date(2027, 9, 16), 1048.0),
)

USDZAR_FORWARD = (
    (date(2026, 9, 16), 18.00),
    (date(2026, 10, 16), 18.15),
    (date(2026, 12, 16), 18.45),
    (date(2027, 3, 16), 18.90),
    (date(2027, 9, 16), 19.80),
)

# One date per contract month. The curve uses the month and year.
BRENT_FORWARD = (
    (date(2026, 10, 1), 75.0),
    (date(2026, 11, 1), 74.5),
    (date(2026, 12, 1), 74.0),
    (date(2027, 1, 1), 73.5),
    (date(2027, 2, 1), 73.0),
    (date(2027, 3, 1), 72.5),
)

SPOTS = {("XAU", "USD"): XAU_SPOT}
FORWARDS = {
    ("XAU", "USD"): GOFO,
    ("XPT", "USD"): XPT_FORWARD,
    ("USDZAR", "ZAR"): USDZAR_FORWARD,
    ("DATED_BRENT", "USD"): BRENT_FORWARD,
}

# Put deltas in percent. 50 is at the money. Wings sit above the at-the-money vol.
DELTAS = (10.0, 25.0, 50.0, 75.0, 90.0)
DELTA_WING = {10.0: 0.04, 25.0: 0.015, 50.0: 0.0, 75.0: 0.015, 90.0: 0.04}

# Moneyness (strike - forward) / forward. Zero is at the money.
MONEYNESS = (-0.1, 0.0, 0.1)
MONEYNESS_WING = {-0.1: 0.03, 0.0: 0.0, 0.1: 0.03}

WINGS = {DeltaVol: DELTA_WING, CalendarMonthMoneynessVol: MONEYNESS_WING}

# At-the-money volatility on the anchor date, before the tenor slope.
ATM_VOLATILITY = {
    ("XAU", "USD"): 0.14,
    ("XAU", "ZAR"): 0.18,
    ("XPT", "USD"): 0.22,
    ("XPT", "ZAR"): 0.26,
    ("USDZAR", "ZAR"): 0.12,
    ("DATED_BRENT", "USD"): 0.30,
}


def year_fraction(day: date, anchor: date = SAMPLE_ANCHOR) -> float:
    """Actual/365 fraction from the anchor to ``day``."""
    return (day - anchor).days / 365.0


def discount_factor(continuous_rate: float, day: date, anchor: date = SAMPLE_ANCHOR) -> float:
    """Continuous Actual/365 discount factor from the anchor to ``day``."""
    return math.exp(-continuous_rate * year_fraction(day, anchor))


def volatility(market: str, currency: str, strike_dimension: float, day: date) -> float:
    """Dummy volatility for one surface point."""
    return _volatility(CATALOGUE.quote_spec(market, currency), strike_dimension, day)


def _volatility(quote: QuoteSpec, strike_dimension: float, day: date) -> float:
    base = _sample(ATM_VOLATILITY, quote, "at-the-money volatility")
    wing = WINGS[type(quote.volatility)][strike_dimension]
    return base + wing + VOL_TENOR_SLOPE * year_fraction(day)


def load_sample(connection: sqlite3.Connection, catalogue: Optional[Catalogue] = None) -> date:
    """Create the tables if needed and insert the dummy anchor date.

    The connection is committed before returning.
    """
    catalogue = catalogue or CATALOGUE
    initialize(connection, catalogue)
    anchor = SAMPLE_ANCHOR.isoformat()
    quotes = catalogue.quotes()
    _insert_spots(connection, anchor, quotes)
    _insert_forwards(connection, anchor, quotes)
    _insert_discounts(connection, anchor, catalogue)
    _insert_volatilities(connection, anchor, quotes)
    connection.commit()
    return SAMPLE_ANCHOR


def _sample(table, quote: QuoteSpec, what: str):
    try:
        return table[(quote.market, quote.currency)]
    except KeyError:
        raise KeyError(
            f"The sample has no {what} for {quote.market} {quote.currency}. "
            "Add it to markets/sample.py."
        ) from None


def _insert_spots(connection: sqlite3.Connection, anchor: str, quotes: list[QuoteSpec]) -> None:
    connection.executemany(
        "INSERT INTO spot (anchor_date, market, currency, value) VALUES (?, ?, ?, ?)",
        [
            (anchor, quote.market, quote.currency, _sample(SPOTS, quote, "spot"))
            for quote in quotes
            if quote.stores_spot
        ],
    )


def _insert_forwards(connection: sqlite3.Connection, anchor: str, quotes: list[QuoteSpec]) -> None:
    rows = []
    for quote in quotes:
        if quote.stores_forward:
            for day, value in _sample(FORWARDS, quote, "forward pillars"):
                rows.append((anchor, quote.market, quote.currency, day.isoformat(), value))
    connection.executemany(
        """
        INSERT INTO forward_pillar (anchor_date, market, currency, pillar_date, value)
        VALUES (?, ?, ?, ?, ?)
        """,
        rows,
    )


def _insert_discounts(connection: sqlite3.Connection, anchor: str, catalogue: Catalogue) -> None:
    rows = []
    for currency in catalogue.discount_curves:
        if currency not in CONTINUOUS_RATES:
            raise KeyError(
                f"The sample has no {currency} discount rate. Add it to markets/sample.py."
            )
        for day in POINT_DATES:
            factor = discount_factor(CONTINUOUS_RATES[currency], day)
            rows.append((anchor, currency, day.isoformat(), factor))
    connection.executemany(
        """
        INSERT INTO discount_pillar (anchor_date, currency, pillar_date, discount_factor)
        VALUES (?, ?, ?, ?)
        """,
        rows,
    )


def _insert_volatilities(connection: sqlite3.Connection, anchor: str, quotes: list[QuoteSpec]) -> None:
    rows = []
    for quote in quotes:
        if isinstance(quote.volatility, CalendarMonthMoneynessVol):
            dates = [day for day, _value in _sample(FORWARDS, quote, "forward pillars")]
            strikes = MONEYNESS
        else:
            dates = list(POINT_DATES)
            strikes = DELTAS
        for day in dates:
            for strike in strikes:
                rows.append(
                    (quote.market, quote.currency, day, strike, _volatility(quote, strike, day))
                )
    connection.executemany(
        """
        INSERT INTO volatility_pillar (
            anchor_date, market, currency, pillar_date, strike_dimension, volatility
        ) VALUES (?, ?, ?, ?, ?, ?)
        """,
        [
            (anchor, market, currency, day.isoformat(), strike, vol)
            for market, currency, day, strike, vol in rows
        ],
    )
