"""One ForwardCurve class covers every averaging and roll combination."""

import math
import unittest
from datetime import date

import numpy as np

import sjd

ANCHOR = date(2026, 9, 16)
PILLARS = [
    ANCHOR,
    date(2026, 12, 16),
    date(2027, 3, 16),
    date(2027, 9, 16),
    date(2028, 9, 18),
]
FORWARDS = [100.0, 104.0, 103.0, 110.0, 112.0]
DELTAS = [10.0, 25.0, 50.0, 75.0, 90.0]


def delta_surface(volatility=0.2):
    grid = [[volatility] * len(PILLARS) for _ in DELTAS]
    return sjd.VolatilitySurfaceDelta(
        ANCHOR, PILLARS, DELTAS, grid, sjd.SurfaceInterpolator.BILINEAR, extrapolate=False
    )


class FlatWingsTest(unittest.TestCase):
    def test_delta_beyond_the_grid_reads_the_edge(self):
        wing_vols = [0.30, 0.25, 0.20, 0.26, 0.35]
        grid = [[vol] * len(PILLARS) for vol in wing_vols]
        for interpolator in (sjd.SurfaceInterpolator.BILINEAR, sjd.SurfaceInterpolator.BICUBIC):
            with self.subTest(interpolator=interpolator):
                surface = sjd.VolatilitySurfaceDelta(
                    ANCHOR, PILLARS, DELTAS, grid, interpolator, extrapolate=False
                )
                self.assertTrue(surface.is_ok(), surface.error_messages())
                expiry = PILLARS[3]
                self.assertAlmostEqual(surface.get_volatility(expiry, 20.0, 100.0), 0.30)
                self.assertAlmostEqual(surface.get_volatility(expiry, 500.0, 100.0), 0.35)

    def test_moneyness_beyond_the_grid_reads_the_edge(self):
        moneyness = [-0.2, 0.0, 0.2]
        grid = [[vol] * len(PILLARS) for vol in (0.30, 0.20, 0.25)]
        surface = sjd.VolatilitySurfaceMoneynessCMA(
            ANCHOR, PILLARS, moneyness, grid, sjd.SurfaceInterpolator.BILINEAR, extrapolate=False
        )
        self.assertTrue(surface.is_ok(), surface.error_messages())
        expiry = date(2027, 3, 20)
        self.assertAlmostEqual(surface.get_volatility(expiry, 10.0, 100.0), 0.30)
        self.assertAlmostEqual(surface.get_volatility(expiry, 400.0, 100.0), 0.25)


class ForwardCurveTest(unittest.TestCase):
    def test_defaults_are_a_linear_point_curve(self):
        curve = sjd.ForwardCurve(ANCHOR, PILLARS, FORWARDS)
        self.assertTrue(curve.is_ok(), curve.error_messages())
        self.assertAlmostEqual(curve.get_forward(PILLARS[1]), FORWARDS[1])
        midpoint = date(2026, 10, 31)
        weight = (midpoint - PILLARS[0]).days / (PILLARS[1] - PILLARS[0]).days
        expected = FORWARDS[0] + weight * (FORWARDS[1] - FORWARDS[0])
        self.assertAlmostEqual(curve.get_forward(midpoint), expected)
        self.assertEqual(curve.anchor_date(), ANCHOR)
        self.assertEqual(curve.final_date(), PILLARS[-1])

    def test_calendar_month_reads_the_month(self):
        months = [ANCHOR, date(2026, 10, 1), date(2026, 11, 1), date(2026, 12, 1)]
        curve = sjd.ForwardCurve(
            ANCHOR,
            months,
            [75.0, 74.5, 74.0, 73.5],
            averaging=sjd.ForwardAveraging.CALENDAR_MONTH,
        )
        self.assertTrue(curve.is_ok(), curve.error_messages())
        self.assertAlmostEqual(
            curve.get_forward(date(2026, 11, 3)), curve.get_forward(date(2026, 11, 27))
        )

    def test_cubic_interpolation_is_passed_through(self):
        linear = sjd.ForwardCurve(ANCHOR, PILLARS, FORWARDS)
        cubic = sjd.ForwardCurve(
            ANCHOR, PILLARS, FORWARDS, interpolation=sjd.CurveInterpolator.CUBIC
        )
        self.assertTrue(cubic.is_ok(), cubic.error_messages())
        self.assertAlmostEqual(cubic.get_forward(PILLARS[2]), FORWARDS[2])
        between = date(2027, 1, 30)
        self.assertNotAlmostEqual(cubic.get_forward(between), linear.get_forward(between))

    def test_calendar_month_has_no_spot_volatility_roll(self):
        with self.assertRaises(ValueError):
            sjd.ForwardCurve(
                ANCHOR,
                PILLARS,
                FORWARDS,
                averaging=sjd.ForwardAveraging.CALENDAR_MONTH,
                roll=sjd.ForwardRoll.SPOT_VOLATILITY,
                volatility=delta_surface(),
            )

    def test_stochastic_roll_reports_missing_volatility(self):
        curve = sjd.ForwardCurve(
            ANCHOR, PILLARS, FORWARDS, roll=sjd.ForwardRoll.FORWARD_VOLATILITY
        )
        report = curve.validate_report()
        self.assertFalse(report.ok)
        codes = [issue.code for issue in report.issues]
        self.assertIn("FORWARD_VOLATILITY_RATE_SOURCE_MISSING", codes)

    def test_monte_carlo_runs_on_a_delta_surface(self):
        for roll in (sjd.ForwardRoll.FORWARD_VOLATILITY, sjd.ForwardRoll.SPOT_VOLATILITY):
            with self.subTest(roll=roll):
                paths, anchor_value = self._seeded_option_run(roll)
                self.assertTrue(np.isfinite(paths).all())
                self.assertAlmostEqual(paths[0, 0], anchor_value, places=6)

    def _seeded_option_run(self, roll):
        surface = delta_surface()
        forward = sjd.ForwardCurve(
            ANCHOR, PILLARS, FORWARDS, roll=roll, volatility=surface
        )
        self.assertTrue(forward.is_ok(), forward.error_messages())
        discount = sjd.DiscountCurve(
            ANCHOR, PILLARS, [math.exp(-0.04 * (day - ANCHOR).days / 365.0) for day in PILLARS]
        )
        environment = sjd.RatesEnvironment(ANCHOR)
        environment.set_frs(forward)
        environment.set_drs(discount)
        environment.set_vrs(surface)

        option = sjd.EuropeanOption(
            maturity=date(2027, 9, 14),
            settlement=PILLARS[3],
            strike=105.0,
            type=sjd.PutCall.CALL,
            volume=1000.0,
            premium_payment_date=PILLARS[3],
            premium_amount=0.0,
            buy_sell=sjd.BuySell.BUY,
        )
        portfolio = sjd.Portfolio([option])
        ready = portfolio.validate_report(environment)
        self.assertTrue(ready.ok, [issue.message for issue in ready.issues])

        simulation = sjd.simulate(
            portfolio, environment, [ANCHOR, PILLARS[1], PILLARS[2]], paths=50, seed=5
        )
        self.assertEqual(simulation.steps, 3)
        return simulation.mtm, portfolio.get_value(environment)


if __name__ == "__main__":
    unittest.main()
