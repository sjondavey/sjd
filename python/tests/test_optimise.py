"""Fixed-path revaluation and collar cost search."""

import unittest

import numpy as np

import sjd
from sjd import examples, optimise, stats, strategy


class ValueSpotPathsTest(unittest.TestCase):
    def setUp(self):
        self.market = examples.sample_market(stochastic=True)
        self.environment = self.market["environment"]
        self.dates = self.market["simulation_dates"]
        self.portfolio = sjd.Portfolio(
            examples.sample_contracts(self.market["forward"], self.market)["contracts"]
        )

    def test_batch_matches_single_paths_and_simulate(self):
        simulation = sjd.simulate(
            self.portfolio, self.environment, self.dates, paths=6, seed=17, antithetic=False
        )
        evolver = optimise.make_evolver(self.portfolio, self.environment, self.dates)
        report = evolver.validate_report()
        self.assertTrue(report.ok, [issue.message for issue in report.issues])

        batch = evolver.value_spot_paths(simulation.spot)
        self.assertEqual(batch.paths, simulation.paths)
        np.testing.assert_allclose(batch.spot, simulation.spot, rtol=1e-10, atol=1e-8)
        np.testing.assert_allclose(batch.mtm, simulation.mtm, rtol=1e-8, atol=1e-4)

        for row in range(simulation.paths):
            single = evolver.value_spot_path(list(simulation.spot[row]))
            np.testing.assert_allclose(single.mtm[0], batch.mtm[row], rtol=1e-10, atol=1e-8)

    def test_common_spots_across_books(self):
        spots = optimise.draw_spot_paths(
            self.portfolio, self.environment, self.dates, paths=8, seed=3, antithetic=False
        )
        collar = strategy.sample_collar(self.market, self.market["forward"])
        first = optimise.value_on_spots(collar.portfolio(), self.environment, self.dates, spots)
        second = optimise.value_on_spots(collar.portfolio(), self.environment, self.dates, spots)
        np.testing.assert_array_equal(first.mtm, second.mtm)
        np.testing.assert_allclose(first.spot, spots)


class StructureCostsTest(unittest.TestCase):
    def setUp(self):
        self.market = examples.sample_market(stochastic=True)
        self.environment = self.market["environment"]
        self.dates = self.market["simulation_dates"]

    def test_costs_include_cva_and_optional_cem(self):
        factors = sjd.reg_cap_factors("commodity")
        credit = examples.credit_parameters()
        collar = strategy.sample_collar(self.market, self.market["forward"])
        portfolio = collar.portfolio_b2cem(factors, credit, True)
        report = portfolio.validate_report(self.environment)
        self.assertTrue(report.ok, [issue.message for issue in report.issues])
        simulation = sjd.simulate(
            portfolio,
            self.environment,
            self.dates,
            paths=12,
            seed=9,
            antithetic=False,
            by_structure=True,
        )
        discount = [
            self.market["discount"].get_discount_factor(day) for day in stats.exposure_dates(simulation)
        ]
        probability = stats.flat_default_probability(self.market["anchor"], stats.exposure_dates(simulation), 0.02)
        loss = [0.6] * len(discount)
        costs = stats.structure_costs(
            simulation,
            discount,
            probability,
            loss,
            netting_sets=portfolio.netting_sets(),
            factors=factors,
            credit=credit,
            capital_cost_rate=0.1,
        )
        self.assertGreaterEqual(costs["cva"], 0.0)
        self.assertLessEqual(costs["dva"], 0.0)
        self.assertIn("expected_capital", costs)
        self.assertAlmostEqual(
            costs["total"],
            costs["cva"] + costs["dva"] + costs["funding"] + costs["capital_cost"],
        )


class CollarSearchTest(unittest.TestCase):
    def test_grid_picks_a_trial_on_fixed_spots(self):
        market = examples.sample_market(stochastic=True)
        environment = market["environment"]
        dates = market["simulation_dates"]
        reference = strategy.sample_collar(market, market["forward"]).portfolio()
        spots = optimise.draw_spot_paths(
            reference, environment, dates, paths=10, seed=5, antithetic=False
        )
        discount = [market["discount"].get_discount_factor(day) for day in dates]
        probability = stats.flat_default_probability(market["anchor"], dates, 0.01)
        loss = [0.45] * len(dates)
        result = optimise.minimise_collar_put_moneyness(
            market,
            market["forward"],
            environment,
            dates,
            spots,
            put_moneyness=[0.94, 0.96, 0.98],
            call_moneyness=1.03,
            volume=1_000_000.0,
            discount_factors=discount,
            default_probability=probability,
            loss_given_default=loss,
            objective="cva",
            premium_weight=1.0,
        )
        self.assertEqual(len(result["trials"]), 3)
        self.assertIn(result["strategy"].put_strike, {
            trial["strategy"].put_strike for trial in result["trials"]
        })
        scores = [trial["score"] for trial in result["trials"]]
        self.assertEqual(result["score"], min(scores))


if __name__ == "__main__":
    unittest.main()
