"""Multi-period strips, the present-value solve, and comparison tables."""

import unittest
from datetime import date

import sjd
from sjd.markets import SAMPLE_ANCHOR, build_market, load_market, load_sample
from sjd.markets.rates import spot_date
from sjd.markets.specs import quote_spec
from sjd.structuring import (
    CollarStrip,
    ForwardStrip,
    ParticipatingForwardStrip,
    SeagullStrip,
    compare_structures,
    plot_structures,
    solve_pv,
    step_ladder,
)


class StructuringTest(unittest.TestCase):
    def setUp(self):
        anchor = date(2026, 9, 16)
        spot_day = spot_date(quote_spec("XAU", "USD"), anchor)
        dates = [anchor, spot_day, date(2026, 10, 16), date(2026, 11, 16), date(2026, 12, 16)]
        volatility = [
            (day, strike, vol)
            for day in (anchor, date(2026, 10, 14), date(2026, 11, 12), date(2026, 12, 16))
            for strike, vol in ((10.0, 0.16), (50.0, 0.14), (90.0, 0.18))
        ]
        self.loaded = build_market(
            "XAU",
            anchor,
            "USD",
            spot=2400.0,
            discount=[(day, 1.0 - 0.0001 * index) for index, day in enumerate(dates)],
            forward=[(date(2026, 10, 16), 0.01), (date(2026, 11, 16), 0.012), (date(2026, 12, 16), 0.013)],
            volatility=volatility,
        )
        self.schedule = self.loaded.monthly_schedule(date(2026, 10, 16), 2)

    def test_a_forward_strip_solves_to_the_target_value(self):
        self.assertEqual(len(self.schedule), 2)
        self.assertEqual(self.schedule.periods[0].expiry, date(2026, 10, 14))
        built = ForwardStrip.build(self.schedule, 2300.0, 100.0)
        self.assertEqual(len(built.contracts()), 2)
        self.assertGreater(built.pv(self.loaded), 0.0)
        solved = solve_pv(
            lambda strike: ForwardStrip.build(self.schedule, strike, 100.0),
            self.loaded,
            target_pv=0.0,
        )
        self.assertAlmostEqual(solved.pv, 0.0, places=4)
        self.assertGreater(solved.parameter, 2300.0)
        self.assertLess(solved.parameter, 2400.0)

    def test_collars_seagulls_and_participating_forwards_group_by_period(self):
        collar = CollarStrip.build(self.schedule, 2300.0, width=100.0, volume=step_ladder(100.0, 25.0, 2, 1))
        self.assertEqual(len(collar.periods), 2)
        self.assertEqual(len(collar.contracts()), 4)
        self.assertEqual(collar.periods[1].contracts[0].volume, 125.0)
        seagull = SeagullStrip.build(self.schedule, 2200.0, 2300.0, 2500.0, volume=50.0)
        participating = ParticipatingForwardStrip.build(
            self.schedule, 2300.0, 2500.0, participation=0.5, volume=80.0
        )
        self.assertEqual(len(seagull.contracts()), 6)
        self.assertEqual(participating.contracts()[1].volume, 40.0)
        for structure in (collar, seagull, participating):
            self.assertEqual(len(structure.period_pvs(self.loaded)), 2)

    def test_asian_strips_use_the_calendar_month(self):
        import sqlite3

        connection = sqlite3.connect(":memory:")
        self.addCleanup(connection.close)
        load_sample(connection)
        loaded = load_market("DATED_BRENT", SAMPLE_ANCHOR, "USD", connection)
        schedule = loaded.monthly_schedule(date(2026, 10, 1), 2)
        self.assertGreater(len(schedule.periods[0].averaging_dates), 10)
        built = ForwardStrip.build(schedule, 70.0, 1000.0)
        self.assertIsInstance(built.contracts()[0], sjd.AsianForward)
        self.assertNotEqual(built.pv(loaded), 0.0)

    def test_comparison_returns_one_row_per_structure(self):
        structures = {
            "forward": ForwardStrip.build(self.schedule, 2400.0, 100.0),
            "collar": CollarStrip.from_moneyness(self.schedule, self.loaded, 0.95, 1.05, volume=100.0),
        }
        summary, schedule = compare_structures(structures, self.loaded, group_every=1)
        self.assertEqual(list(summary.columns), ["group", "forward_pv", "collar_pv"])
        self.assertEqual(len(schedule), 2)
        self.assertIn("collar_put", list(schedule.columns))
        figure = plot_structures(structures, self.loaded)
        self.assertEqual(len(figure.axes), 2)
        figure.clf()


if __name__ == "__main__":
    unittest.main()
