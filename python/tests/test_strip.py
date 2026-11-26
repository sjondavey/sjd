"""Bootstrap USD deposits, gold lease rates, and delta smiles."""

import unittest
from datetime import date

import sjd
from sjd.markets import CATALOGUE
from sjd.markets.rates import calendar
from sjd.markets.strip import (
    advance_tenor,
    delta_pillars_from_rr_bf,
    parse_tenor,
    strip_discount_curve,
    strip_gofo_to_lease,
    strip_usd_deposits,
    wing_vols,
)

ANCHOR = date(2026, 9, 16)
DEPOSITS = {
    "1W": 0.0430,
    "1M": 0.0432,
    "2M": 0.0430,
    "3M": 0.0428,
    "6M": 0.0420,
    "1Y": 0.0405,
    "2Y": 0.0385,
    "3Y": 0.0380,
    "5Y": 0.0390,
}
GOFO = {
    "1M": 0.0380,
    "2M": 0.0375,
    "3M": 0.0370,
    "6M": 0.0355,
    "1Y": 0.0335,
    "2Y": 0.0310,
    "3Y": 0.0300,
    "5Y": 0.0305,
}
VOLS = [
    ("1M", 0.140, -0.010, 0.003, -0.022, 0.012),
    ("3M", 0.145, -0.012, 0.004, -0.026, 0.014),
    ("6M", 0.150, -0.014, 0.005, -0.030, 0.016),
    ("3Y", 0.160, -0.012, 0.006, -0.028, 0.018),
    ("5Y", 0.165, -0.010, 0.007, -0.024, 0.020),
]


class StripTest(unittest.TestCase):
    def test_tenors_follow_the_calendar(self):
        self.assertEqual(parse_tenor("1W"), (1, sjd.TimeUnit.WEEKS))
        holidays = calendar(CATALOGUE.quote_spec("XAU", "USD").dates.calendar)
        self.assertEqual(advance_tenor(holidays, ANCHOR, "1M"), date(2026, 10, 16))

    def test_wings_split_the_risk_reversal(self):
        put, call = wing_vols(0.140, -0.010, 0.003)
        self.assertAlmostEqual(call - put, -0.010)
        self.assertAlmostEqual(0.5 * (put + call) - 0.140, 0.003)

    def test_usd_deposits_match_the_notebook_pillars(self):
        curve = strip_usd_deposits(ANCHOR, DEPOSITS)
        self.assertAlmostEqual(curve.df(date(2026, 9, 25)), 0.998926, places=6)
        self.assertLess(curve.df(date(2027, 9, 16)), curve.df(date(2026, 10, 19)))

    def test_swaps_extend_the_same_bootstrap(self):
        curve = strip_discount_curve(ANCHOR, deposits={"3M": 0.0428}, swaps={"2Y": 0.0385})
        self.assertGreater(curve.df(date(2028, 9, 18)), 0.8)
        self.assertLess(curve.df(date(2028, 9, 18)), 1.0)

    def test_gofo_becomes_a_lease_and_keeps_the_forward(self):
        discount = strip_usd_deposits(ANCHOR, DEPOSITS)
        stripped = strip_gofo_to_lease(ANCHOR, 2400.0, GOFO, discount, CATALOGUE.quote_spec("XAU", "USD"))
        row = stripped.frame().iloc[0]
        self.assertEqual(row["value_date"], date(2026, 10, 19))
        self.assertAlmostEqual(row["lease"], 0.0052, places=4)
        self.assertAlmostEqual(row["forward"], 2407.8533, places=4)
        self.assertIn(date(2026, 9, 18), stripped.discount_pillars)
        last = stripped.frame().iloc[-1]
        self.assertAlmostEqual(last["forward"], 2771.2867, places=4)

    def test_smiles_are_stored_as_put_delta_and_include_the_anchor(self):
        pillars = delta_pillars_from_rr_bf(ANCHOR, VOLS, CATALOGUE.quote_spec("XAU", "USD")).rows()
        strikes = {row[1] for row in pillars}
        self.assertEqual(strikes, {10.0, 25.0, 50.0, 75.0, 90.0})
        anchor_atm = next(row[2] for row in pillars if row[0] == ANCHOR and row[1] == 50.0)
        self.assertAlmostEqual(anchor_atm, 0.140)
        self.assertEqual(len(pillars), 6 * 5)


if __name__ == "__main__":
    unittest.main()
