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
        self.assertEqual(collar.client, strategy.Client.PRODUCER)
        self.assertEqual(put.buy_sell, sjd.BuySell.SELL)
        self.assertEqual(call.buy_sell, sjd.BuySell.BUY)

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
            call.get_option_value(self.environment) - put.get_option_value(self.environment),
        )

    def test_consumer_is_the_other_side_of_producer(self):
        producer = strategy.sample_collar(self.market, self.forward)
        consumer = strategy.sample_collar(
            self.market, self.forward, client=strategy.Client.CONSUMER
        )
        producer_put, producer_call = producer.contracts()
        consumer_put, consumer_call = consumer.contracts()
        self.assertEqual(consumer_put.buy_sell, sjd.BuySell.BUY)
        self.assertEqual(consumer_call.buy_sell, sjd.BuySell.SELL)
        producer.portfolio().validate(self.environment)
        consumer.portfolio().validate(self.environment)
        self.assertAlmostEqual(
            producer.portfolio().get_value(self.environment),
            -consumer.portfolio().get_value(self.environment),
        )
        with self.assertRaises(ValueError):
            strategy.CollarStrategy(
                maturity=self.market["collar_maturity"],
                settlement=self.market["collar_settlement"],
                put_strike=17.0,
                call_strike=18.0,
                volume=1.0,
                client="producer",
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

    def test_strip_seagull_and_participating_forward_price(self):
        environment = self.environment
        forward = self.forward
        settlement = self.market["forward_settlement"]
        strip = strategy.StripStrategy.from_moneyness(
            [self.market["collar_settlement"], settlement], forward, 1.0, 10.0
        )
        self.assertEqual(len(strip.contracts()), 2)
        self.assertEqual(strip.premium(environment), 0.0)
        strip_book = strip.portfolio()
        report = strip_book.validate_report(environment)
        self.assertTrue(report.ok, [issue.message for issue in report.issues])
        self.assertAlmostEqual(
            strip_book.get_value(environment),
            sum(contract.get_value(environment) for contract in strip.contracts()),
        )

        atm = forward.get_forward(self.market["collar_settlement"])
        seagull = strategy.SeagullStrategy.from_moneyness(
            self.market["collar_maturity"],
            self.market["collar_settlement"],
            atm,
            0.90,
            0.97,
            1.03,
            1_000_000.0,
        )
        collar = strategy.sample_collar(self.market, forward)
        self.assertEqual(len(seagull.contracts()), 3)
        put, call, floor = seagull.contracts()
        self.assertEqual(put.buy_sell, sjd.BuySell.SELL)
        self.assertEqual(call.buy_sell, sjd.BuySell.BUY)
        self.assertEqual(floor.buy_sell, sjd.BuySell.BUY)
        self.assertAlmostEqual(
            seagull.premium(environment),
            -put.get_option_value(environment)
            + call.get_option_value(environment)
            + floor.get_option_value(environment),
        )
        # The bank is long the floor put, so the seagull is worth more than the collar.
        seagull.portfolio().validate(environment)
        collar.portfolio().validate(environment)
        self.assertGreater(
            seagull.portfolio().get_value(environment),
            collar.portfolio().get_value(environment),
        )
        self.assertTrue(seagull.portfolio().validate_report(environment).ok)

        participating = strategy.ParticipatingForwardStrategy.from_moneyness(
            self.market["collar_maturity"],
            self.market["collar_settlement"],
            atm,
            0.97,
            1.03,
            1_000.0,
            0.5,
        )
        self.assertAlmostEqual(participating.call_volume(), 500.0)
        self.assertEqual(participating.contracts()[0].buy_sell, sjd.BuySell.SELL)
        self.assertEqual(participating.contracts()[1].buy_sell, sjd.BuySell.BUY)
        self.assertEqual(participating.contracts()[1].volume, 500.0)
        self.assertTrue(participating.portfolio().validate_report(environment).ok)

        with self.assertRaises(ValueError):
            strategy.SeagullStrategy.from_moneyness(
                self.market["collar_maturity"],
                self.market["collar_settlement"],
                atm,
                0.97,
                0.90,
                1.03,
                1.0,
            )


if __name__ == "__main__":
    unittest.main()
