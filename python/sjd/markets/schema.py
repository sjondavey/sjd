"""Create the market-data tables in a SQLite database."""

import sqlite3
from pathlib import Path

from .specs import CATALOGUE, Catalogue

SCHEMA_PATH = Path(__file__).with_name("schema.sql")
SCHEMA_VERSION = 2


class SchemaVersionError(RuntimeError):
    """The database was made by an older schema and has to be rebuilt."""


def schema_sql() -> str:
    """Return the CREATE statements in ``schema.sql``."""
    return SCHEMA_PATH.read_text(encoding="utf-8")


def initialize(connection: sqlite3.Connection, catalogue: Catalogue | None = None) -> None:
    """Create the tables if needed, then add the catalogue's quotes to the key tables.

    Quotes are only added. A quote removed from the catalogue keeps its rows,
    because its pillars may still point at them. ``executescript`` commits
    the connection before it runs, and this function commits at the end.
    Foreign keys are switched on for ``connection``. SQLite does not
    remember that, so another connection must run ``PRAGMA foreign_keys = ON``
    itself before writing.
    """
    catalogue = catalogue or CATALOGUE
    existing = connection.execute(
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'schema_info'"
    ).fetchone()
    if existing is not None:
        version = connection.execute("SELECT version FROM schema_info").fetchone()[0]
        if version != SCHEMA_VERSION:
            raise SchemaVersionError(
                f"The database is schema version {version}, and this code needs "
                f"{SCHEMA_VERSION}. Rebuild it, for example by running the "
                "sample_database notebook."
            )
    connection.executescript(schema_sql())
    connection.executemany(
        "INSERT OR IGNORE INTO currency (code) VALUES (?)",
        [(code,) for code in catalogue.discount_curves],
    )
    quotes = catalogue.quotes()
    connection.executemany(
        "INSERT OR IGNORE INTO market_quote (market, currency) VALUES (?, ?)",
        [(quote.market, quote.currency) for quote in quotes],
    )
    connection.executemany(
        "INSERT OR IGNORE INTO stored_forward (market, currency) VALUES (?, ?)",
        [(quote.market, quote.currency) for quote in quotes if quote.stores_forward],
    )
    connection.executemany(
        "INSERT OR IGNORE INTO stored_spot (market, currency) VALUES (?, ?)",
        [(quote.market, quote.currency) for quote in quotes if quote.stores_spot],
    )
    connection.commit()
