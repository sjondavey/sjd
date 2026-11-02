"""Parametrised strategies materialise into priced contracts."""

import unittest

import sjd
from sjd import examples, strategy


class CollarStrategyTest(unittest.TestCase):
    def setUp(self):
        self.market = examples.sample_market()
        self.environment = self.market["environment"]
        self.forward = self.market["forward"]

    def test_sample_collar_matches_example_strikes(self):
        book = examples.sample_contracts(self.forward, self.market)
        collar = strategy.sample_collar(self.market, self.forward)
        self.assertEqual(collar.put_strike, book["put_strike"])
        self.assertEqual(collar.call_strike, book["call_strike"])
        put, call = collar.contracts()
        self.assertEqual(put.maturity_date(), self.market["collar_maturity"])
        self.assertEqual(call.settlement_date(), self.market["collar_settlement"])
        self.assertEqual(put.buy_sell, sjd.BuySell.BUY)
        self.assertEqual(call.buy_sell, sjd.BuySell.SELL)

    def test_portfolio_validates_and_prices(self):
        collar = strategy.sample_collar(self.market, self.forward)
        portfolio = collar.portfolio()
        report = portfolio.validate_report(self.environment)
        self.assertTrue(report.ok, [issue.message for issue in report.issues])
        put, call = collar.contracts()
        put.validate(self.environment)
        call.validate(self.environment)
        self.assertAlmostEqual(
            portfolio.get_value(self.environment),
            put.get_value(self.environment) + call.get_value(self.environment),
        )
        self.assertAlmostEqual(
            collar.premium(self.environment),
            put.get_option_value(self.environment) + call.get_option_value(self.environment),
        )

    def test_rejects_inverted_wings(self):
        with self.assertRaises(ValueError):
            strategy.CollarStrategy(
                maturity=self.market["collar_maturity"],
                settlement=self.market["collar_settlement"],
                put_strike=20.0,
                call_strike=18.0,
                volume=1.0,
            )


if __name__ == "__main__":
    unittest.main()
