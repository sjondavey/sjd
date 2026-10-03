"""sjd.simulate returns every path, and sjd.stats summarises them."""

import unittest
from datetime import date
from types import SimpleNamespace

import numpy as np

import sjd
from sjd import examples, stats


def _column(detail, heading):
    index = list(detail["headings"]).index(heading)
    return np.array([row[index] for row in detail["rows"]])


class SimulateTest(unittest.TestCase):
    def setUp(self):
        self.market = examples.sample_market(stochastic=True)
        self.environment = self.market["environment"]
        self.dates = self.market["simulation_dates"]

    def _contracts(self):
        return examples.sample_contracts(self.market["forward"], self.market)["contracts"]

    def _portfolio(self):
        portfolio = sjd.Portfolio(self._contracts())
        report = portfolio.validate_report(self.environment)
        self.assertTrue(report.ok, [issue.message for issue in report.issues])
        return portfolio

    def _margined(self):
        margin = sjd.SimpleMargin(self.market["anchor"], True, 50_000.0, True, 50_000.0, 10_000.0, 1_000.0)
        portfolio = sjd.PortfolioB2CEM(
            self._contracts(),
            sjd.reg_cap_factors("commodity"),
            examples.credit_parameters(),
            True,
            margin,
        )
        report = portfolio.validate_report(self.environment)
        self.assertTrue(report.ok, [issue.message for issue in report.issues])
        return portfolio

    def test_arrays_are_shaped_paths_by_steps(self):
        simulation = sjd.simulate(self._portfolio(), self.environment, self.dates, paths=21, seed=3)
        self.assertEqual(simulation.paths, 22)
        self.assertEqual(simulation.steps, len(self.dates))
        self.assertTrue(simulation.antithetic)
        self.assertEqual(simulation.dates, self.dates)
        self.assertEqual(simulation.spot.shape, (22, len(self.dates)))
        self.assertEqual(simulation.mtm.shape, (22, len(self.dates)))
        self.assertIsNone(simulation.margin)
        self.assertIsNone(simulation.by_trade)
        self.assertIs(simulation.mtm, simulation.mtm)
        np.testing.assert_array_equal(simulation.exposure_steps, np.arange(len(self.dates)))
        np.testing.assert_array_equal(simulation.margin_reset_steps, simulation.exposure_steps)

        independent = sjd.simulate(
            self._portfolio(), self.environment, self.dates, paths=21, seed=3, antithetic=False
        )
        self.assertEqual(independent.paths, 21)
        self.assertFalse(independent.antithetic)

    def test_every_path_starts_at_todays_value(self):
        portfolio = self._portfolio()
        simulation = sjd.simulate(portfolio, self.environment, self.dates, paths=8, seed=5)
        np.testing.assert_allclose(simulation.mtm[:, 0], portfolio.get_value(self.environment))
        np.testing.assert_allclose(simulation.spot[:, 0], self.market["forward"].get_forward(self.dates[0]))

    def test_a_seed_repeats_the_paths(self):
        first = sjd.simulate(self._portfolio(), self.environment, self.dates, paths=10, seed=7)
        second = sjd.simulate(self._portfolio(), self.environment, self.dates, paths=10, seed=7)
        other = sjd.simulate(self._portfolio(), self.environment, self.dates, paths=10, seed=8)
        np.testing.assert_array_equal(first.mtm, second.mtm)
        np.testing.assert_array_equal(first.spot, second.spot)
        self.assertFalse(np.array_equal(first.mtm[:, 1:], other.mtm[:, 1:]))

    def test_means_match_the_evolver_for_the_same_seed(self):
        portfolio = self._portfolio()
        generator = sjd.SingleFactorEconomicScenarioGenerator(self.environment)
        evolver = sjd.PortfolioEvolver(portfolio, generator, self.dates)
        generator.set_random_seed(11)
        evolver.run_monte_carlo(20)
        detail = evolver.get_monte_carlo_detail()

        simulation = sjd.simulate(portfolio, self.environment, self.dates, paths=20, seed=11)
        values = stats.exposure(simulation)
        np.testing.assert_allclose(simulation.spot.mean(axis=0), _column(detail, "Ave Path"))
        np.testing.assert_allclose(simulation.mtm.mean(axis=0), _column(detail, "Ave MtM"))
        np.testing.assert_allclose(stats.expected_exposure(values), _column(detail, "EE"))
        np.testing.assert_allclose(stats.negative_expected_exposure(values), _column(detail, "NEE"))

    def test_margin_is_returned_and_reduces_exposure(self):
        portfolio = self._margined()
        generator = sjd.SingleFactorEconomicScenarioGenerator(self.environment)
        evolver = sjd.PortfolioEvolver(portfolio, generator, self.dates)
        generator.set_random_seed(13)
        evolver.run_monte_carlo(20)
        detail = evolver.get_monte_carlo_detail()

        simulation = sjd.simulate(portfolio, self.environment, self.dates, paths=20, seed=13)
        self.assertEqual(simulation.margin.shape, simulation.mtm.shape)
        values = stats.exposure(simulation)
        np.testing.assert_allclose(values, simulation.mtm - simulation.margin)
        np.testing.assert_allclose(simulation.margin.mean(axis=0), _column(detail, "Ave Margin"))
        np.testing.assert_allclose(stats.expected_exposure(values), _column(detail, "EE"))
        np.testing.assert_allclose(stats.holding_period_exposure(simulation), values)

    def test_holding_period_matches_the_evolver(self):
        portfolio = self._margined()
        usdzar = sjd.Calendar("USDZAR", ["UnitedStates", "SouthAfrica"])
        generator = sjd.SingleFactorEconomicScenarioGenerator(self.environment)
        evolver = sjd.PortfolioEvolver(
            portfolio, generator, self.dates, holding_period=10, holding_period_calendar=usdzar
        )
        report = evolver.validate_report()
        self.assertTrue(report.ok, [issue.message for issue in report.issues])
        self.assertEqual(evolver.exposure_dates, self.dates)
        reset = usdzar.advance(self.dates[1], -10)
        self.assertIn(reset, evolver.simulation_dates)
        generator.set_random_seed(13)
        evolver.run_monte_carlo(20)
        detail = evolver.get_monte_carlo_detail()
        headings = list(detail["headings"])
        rows = [row for day, row in zip(detail["dates"], detail["rows"]) if day in self.dates]
        ee_with_hp = [row[headings.index("EE with HP")] for row in rows]
        nee_with_hp = [row[headings.index("NEE with HP")] for row in rows]

        simulation = sjd.simulate(
            portfolio, self.environment, self.dates, paths=20, seed=13,
            holding_period=10, holding_period_calendar=usdzar,
        )
        self.assertEqual(simulation.dates, evolver.simulation_dates)
        self.assertEqual(stats.exposure_dates(simulation), self.dates)
        self.assertEqual(simulation.dates[simulation.margin_reset_steps[1]], reset)
        values = stats.holding_period_exposure(simulation)
        self.assertEqual(values.shape, (20, len(self.dates)))
        np.testing.assert_allclose(stats.expected_exposure(values), ee_with_hp)
        np.testing.assert_allclose(stats.negative_expected_exposure(values), nee_with_hp)
        table = stats.summary(simulation, holding_period=True)
        self.assertEqual(table["dates"], self.dates)
        np.testing.assert_allclose(table["ee"], ee_with_hp)

        with self.assertRaisesRegex(ValueError, "holding_period_calendar"):
            sjd.simulate(portfolio, self.environment, self.dates, paths=4, holding_period=10)

    def test_trade_values_sum_to_the_portfolio(self):
        simulation = sjd.simulate(
            self._portfolio(), self.environment, self.dates, paths=6, seed=2, by_trade=True
        )
        self.assertEqual(simulation.trades, 4)
        self.assertEqual(simulation.by_trade.shape, (6, len(self.dates), 4))
        np.testing.assert_allclose(simulation.by_trade.sum(axis=2), simulation.mtm, atol=1e-6)

    def test_bad_requests_raise_value_error(self):
        with self.assertRaises(ValueError):
            sjd.simulate(self._portfolio(), self.environment, self.dates, paths=0)
        with self.assertRaises(ValueError):
            sjd.simulate(self._portfolio(), self.environment, self.dates[1:], paths=4)

        deterministic = examples.sample_market()
        with self.assertRaisesRegex(ValueError, "stochastic"):
            sjd.simulate(
                sjd.Portfolio(examples.sample_contracts(deterministic["forward"], deterministic)["contracts"]),
                deterministic["environment"],
                deterministic["simulation_dates"],
                paths=4,
            )

    def test_summary_has_one_row_per_date(self):
        simulation = sjd.simulate(self._margined(), self.environment, self.dates, paths=50, seed=1)
        table = stats.summary(simulation)
        self.assertEqual(table["dates"], self.dates)
        for key in ("spot", "mtm", "margin", "ee", "nee", "pfe", "negative_pfe", "ee_standard_error"):
            self.assertEqual(table[key].shape, (len(self.dates),), key)
        self.assertTrue(np.all(table["pfe"] >= 0.0))
        self.assertTrue(np.all(table["negative_pfe"] <= 0.0))


class StatisticsTest(unittest.TestCase):
    values = np.array([
        [0.0, 2.0],
        [0.0, -4.0],
        [0.0, 6.0],
        [0.0, -8.0],
    ])

    def test_expected_exposures(self):
        np.testing.assert_allclose(stats.expected_exposure(self.values), [0.0, 2.0])
        np.testing.assert_allclose(stats.negative_expected_exposure(self.values), [0.0, -3.0])

    def test_potential_future_exposures(self):
        np.testing.assert_allclose(
            stats.potential_future_exposure(self.values, 0.5, method="lower"), [0.0, 0.0]
        )
        np.testing.assert_allclose(
            stats.potential_future_exposure(self.values, 0.9, method="higher"), [0.0, 6.0]
        )
        np.testing.assert_allclose(
            stats.negative_potential_future_exposure(self.values, 0.9, method="lower"), [0.0, -8.0]
        )
        self.assertEqual(stats.quantiles(self.values, [0.25, 0.75]).shape, (2, 2))
        with self.assertRaises(ValueError):
            stats.potential_future_exposure(self.values, 1.0)

    def test_value_at_risk_and_expected_shortfall(self):
        self.assertEqual(stats.value_at_risk(self.values, 1, 0.75, method="lower"), 8.0)
        self.assertEqual(stats.value_at_risk(self.values, 1, 0.5, method="lower"), 4.0)
        self.assertEqual(stats.expected_shortfall(self.values, 1, 0.5, method="lower"), 6.0)
        with self.assertRaises(ValueError):
            stats.value_at_risk(self.values, 0)

    def test_antithetic_standard_error_uses_pair_means(self):
        independent = stats.standard_error(self.values)
        self.assertAlmostEqual(independent[1], np.std([2.0, -4.0, 6.0, -8.0], ddof=1) / 2.0)
        pairs = stats.standard_error(self.values, antithetic=True)
        self.assertAlmostEqual(pairs[1], np.std([4.0, -6.0], ddof=1) / np.sqrt(2.0))
        with self.assertRaises(ValueError):
            stats.standard_error(self.values[:3], antithetic=True)

    def test_exposure_without_margin_is_a_copy(self):
        simulation = SimpleNamespace(mtm=self.values, margin=None)
        result = stats.exposure(simulation)
        result[0, 0] = 99.0
        self.assertEqual(self.values[0, 0], 0.0)


if __name__ == "__main__":
    unittest.main()
