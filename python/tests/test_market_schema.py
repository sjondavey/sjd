"""The SQLite pillar tables accept what the market specs store."""

import sqlite3
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from markets.schema import SCHEMA_VERSION, SchemaVersionError, initialize
from markets.specs import DISCOUNT_CURVES, MARKETS

ANCHOR = "2026-09-16"
PILLAR = "2026-12-16"


class MarketSchemaTest(unittest.TestCase):
    def setUp(self):
        self.connection = sqlite3.connect(":memory:")
        initialize(self.connection)

    def tearDown(self):
        self.connection.close()

    def test_schema_version(self):
        version = self.connection.execute("SELECT version FROM schema_info").fetchone()[0]
        self.assertEqual(version, SCHEMA_VERSION)
        initialize(self.connection)
        self.assertEqual(
            self.connection.execute("SELECT COUNT(*) FROM schema_info").fetchone()[0],
            1,
        )
        quotes = sum(len(market.quotes) for market in MARKETS.values())
        self.assertEqual(
            self.connection.execute("SELECT COUNT(*) FROM market_quote").fetchone()[0],
            quotes,
        )

    def test_an_older_database_is_refused(self):
        old = sqlite3.connect(":memory:")
        self.addCleanup(old.close)
        old.executescript(
            "CREATE TABLE schema_info (id INTEGER PRIMARY KEY, version INTEGER NOT NULL);"
            "INSERT INTO schema_info VALUES (1, 1);"
        )
        with self.assertRaises(SchemaVersionError):
            initialize(old)
        tables = {row[0] for row in old.execute("SELECT name FROM sqlite_master")}
        self.assertEqual(tables, {"schema_info"})

    def test_a_quote_outside_the_catalogue_is_refused(self):
        with self.assertRaises(sqlite3.IntegrityError):
            self._insert_forward("XAG", "USD", 25.0)
        with self.assertRaises(sqlite3.IntegrityError):
            self.connection.execute(
                """
                INSERT INTO volatility_pillar (
                    anchor_date, market, currency, pillar_date, strike_dimension, volatility
                ) VALUES (?, 'XAU', 'EUR', ?, 50.0, 0.12)
                """,
                (ANCHOR, PILLAR),
            )

    def test_forward_rows_follow_the_spec(self):
        for market in MARKETS.values():
            for quote in market.quotes.values():
                with self.subTest(market=quote.market, currency=quote.currency):
                    if quote.stores_forward:
                        self._insert_forward(quote.market, quote.currency, -0.002)
                    else:
                        with self.assertRaises(sqlite3.IntegrityError):
                            self._insert_forward(quote.market, quote.currency, 1.0)
                        self.connection.rollback()

    def test_volatility_rows_exist_for_every_quote(self):
        for market in MARKETS.values():
            for quote in market.quotes.values():
                with self.subTest(market=quote.market, currency=quote.currency):
                    self.connection.execute(
                        """
                        INSERT INTO volatility_pillar (
                            anchor_date, market, currency, pillar_date,
                            strike_dimension, volatility
                        ) VALUES (?, ?, ?, ?, ?, ?)
                        """,
                        (ANCHOR, quote.market, quote.currency, PILLAR, 50.0, 0.12),
                    )

    def test_discount_rows_follow_the_currency_curves(self):
        for currency in DISCOUNT_CURVES:
            self.connection.execute(
                """
                INSERT INTO discount_pillar (
                    anchor_date, currency, pillar_date, discount_factor
                ) VALUES (?, ?, ?, ?)
                """,
                (ANCHOR, currency, PILLAR, 0.99),
            )
        with self.assertRaises(sqlite3.IntegrityError):
            self.connection.execute(
                """
                INSERT INTO discount_pillar (
                    anchor_date, currency, pillar_date, discount_factor
                ) VALUES (?, 'EUR', ?, 0.99)
                """,
                (ANCHOR, PILLAR),
            )

    def test_spot_is_only_the_gofo_quote(self):
        for market in MARKETS.values():
            for quote in market.quotes.values():
                with self.subTest(market=quote.market, currency=quote.currency):
                    if quote.stores_spot:
                        self.connection.execute(
                            """
                            INSERT INTO spot (anchor_date, market, currency, value)
                            VALUES (?, ?, ?, ?)
                            """,
                            (ANCHOR, quote.market, quote.currency, 2300.0),
                        )
                    else:
                        with self.assertRaises(sqlite3.IntegrityError):
                            self.connection.execute(
                                """
                                INSERT INTO spot (anchor_date, market, currency, value)
                                VALUES (?, ?, ?, ?)
                                """,
                                (ANCHOR, quote.market, quote.currency, 2300.0),
                            )
                        self.connection.rollback()

    def test_a_pillar_date_is_unique(self):
        self._insert_forward("XAU", "USD", 0.01)
        with self.assertRaises(sqlite3.IntegrityError):
            self._insert_forward("XAU", "USD", 0.02)

    def _insert_forward(self, market, currency, value):
        self.connection.execute(
            """
            INSERT INTO forward_pillar (
                anchor_date, market, currency, pillar_date, value
            ) VALUES (?, ?, ?, ?, ?)
            """,
            (ANCHOR, market, currency, PILLAR, value),
        )


if __name__ == "__main__":
    unittest.main()
