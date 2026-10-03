"""The dummy anchor date fills the tables the specs can price from."""

import math
import sqlite3
import sys
import unittest
from datetime import date
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from markets.sample import (
    ATM_VOLATILITY,
    BRENT_FORWARD,
    GOFO,
    SAMPLE_ANCHOR,
    USD_CONTINUOUS_RATE,
    XAU_SPOT,
    XPT_FORWARD,
    ZAR_CONTINUOUS_RATE,
    discount_factor,
    load_sample,
    volatility,
)


class MarketSampleTest(unittest.TestCase):
    def setUp(self):
        self.connection = sqlite3.connect(":memory:")
        self.anchor = load_sample(self.connection)

    def tearDown(self):
        self.connection.close()

    def test_anchor_is_the_notebook_date(self):
        self.assertEqual(self.anchor, date(2026, 9, 16))
        self.assertEqual(self.anchor, SAMPLE_ANCHOR)

    def test_stored_forwards_and_the_gold_spot(self):
        self.assertEqual(self._forward("XAU", "USD", "2026-12-16"), GOFO[1][1])
        self.assertEqual(self._forward("XPT", "USD", "2026-09-16"), XPT_FORWARD[0][1])
        self.assertEqual(self._forward("DATED_BRENT", "USD", "2026-10-01"), BRENT_FORWARD[0][1])
        self.assertEqual(self._forward("USDZAR", "ZAR", "2026-09-16"), 18.0)
        self.assertEqual(self._count("forward_pillar", "XAU", "ZAR"), 0)
        self.assertEqual(self._count("forward_pillar", "XPT", "ZAR"), 0)
        spot = self.connection.execute(
            "SELECT value FROM spot WHERE anchor_date = ? AND market = 'XAU'",
            (SAMPLE_ANCHOR.isoformat(),),
        ).fetchone()[0]
        self.assertEqual(spot, XAU_SPOT)

    def test_discount_factors_are_continuous_actual_365(self):
        for currency, rate in (("USD", USD_CONTINUOUS_RATE), ("ZAR", ZAR_CONTINUOUS_RATE)):
            on_anchor = self._discount(currency, SAMPLE_ANCHOR.isoformat())
            one_year = self._discount(currency, "2027-09-16")
            self.assertEqual(on_anchor, 1.0)
            self.assertAlmostEqual(one_year, discount_factor(rate, date(2027, 9, 16)))
            self.assertAlmostEqual(one_year, math.exp(-rate * 365 / 365))

    def test_each_quote_has_its_own_volatility_surface(self):
        gold_usd = volatility("XAU", "USD", 50.0, SAMPLE_ANCHOR)
        gold_zar = volatility("XAU", "ZAR", 50.0, SAMPLE_ANCHOR)
        self.assertEqual(self._volatility("XAU", "USD", "2026-09-16", 50.0), gold_usd)
        self.assertEqual(self._volatility("XAU", "ZAR", "2026-09-16", 50.0), gold_zar)
        self.assertGreater(gold_zar, gold_usd)
        wing = self._volatility("XAU", "USD", "2026-09-16", 10.0)
        self.assertGreater(wing, gold_usd)
        brent_month = date(2026, 10, 1)
        brent_atm = self._volatility("DATED_BRENT", "USD", "2026-10-01", 0.0)
        self.assertAlmostEqual(brent_atm, volatility("DATED_BRENT", "USD", 0.0, brent_month))
        self.assertGreater(brent_atm, ATM_VOLATILITY[("DATED_BRENT", "USD")])
        later = self._volatility("USDZAR", "ZAR", "2027-09-16", 50.0)
        self.assertGreater(later, volatility("USDZAR", "ZAR", 50.0, SAMPLE_ANCHOR))

    def _forward(self, market, currency, pillar):
        return self.connection.execute(
            """
            SELECT value FROM forward_pillar
            WHERE anchor_date = ? AND market = ? AND currency = ? AND pillar_date = ?
            """,
            (SAMPLE_ANCHOR.isoformat(), market, currency, pillar),
        ).fetchone()[0]

    def _discount(self, currency, pillar):
        return self.connection.execute(
            """
            SELECT discount_factor FROM discount_pillar
            WHERE anchor_date = ? AND currency = ? AND pillar_date = ?
            """,
            (SAMPLE_ANCHOR.isoformat(), currency, pillar),
        ).fetchone()[0]

    def _volatility(self, market, currency, pillar, strike):
        return self.connection.execute(
            """
            SELECT volatility FROM volatility_pillar
            WHERE anchor_date = ? AND market = ? AND currency = ?
              AND pillar_date = ? AND strike_dimension = ?
            """,
            (SAMPLE_ANCHOR.isoformat(), market, currency, pillar, strike),
        ).fetchone()[0]

    def _count(self, table, market, currency):
        return self.connection.execute(
            f"SELECT COUNT(*) FROM {table} WHERE market = ? AND currency = ?",
            (market, currency),
        ).fetchone()[0]


if __name__ == "__main__":
    unittest.main()
