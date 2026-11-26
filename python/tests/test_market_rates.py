"""Rate functions turn the dummy pillars into a RatesEnvironment."""

import sqlite3
import tempfile
import unittest
from datetime import date, datetime
from pathlib import Path

import numpy as np

import sjd
from sjd.markets.rates import (
    build_market,
    business_days_after,
    calendar,
    load_market,
    month_settlement,
    open_market,
    option_expiry,
    pricing_dates,
    rates_environment,
    save_market_pillars,
    spot_date,
)
from sjd.markets.sample import (
    SAMPLE_ANCHOR,
    SAMPLE_QUOTES,
    USD_CONTINUOUS_RATE,
    USDZAR_FORWARD,
    XAU_SPOT,
    XPT_FORWARD,
    ZAR_CONTINUOUS_RATE,
    discount_factor,
    load_sample,
)
from sjd.markets.schema import initialize
from sjd.markets.specs import (
    CATALOGUE,
    MARKETS,
    CalendarSpec,
    ForwardAveraging,
    ForwardRoll,
    QuoteNotAvailable,
    parse_catalogue,
    quote_spec,
)
from sjd.markets.store import RatesLabelError, discount_pillars, forward_pillars, volatility_grid

VALUE = date(2026, 12, 16)
GOFO_DEC = 0.008
SIMULATION_DATES = [SAMPLE_ANCHOR, date(2026, 11, 16), date(2027, 1, 15)]

# A market that is not in catalogue.toml. Adding it takes this text and its pillars only.
SILVER_CATALOGUE = """
[calendars.LONDON_METALS]
name = "UnitedKingdom.Metals"

[dates.LOCO_LONDON]
kind = "point"
calendar = "LONDON_METALS"
spot_lag_business_days = 2
expiry_before_value_business_days = 2

[discount.USD]
interpolation = "LINEAR"
extrapolate = false

[markets.XAG]
summary = "Loco London silver outright."

[markets.XAG.quotes.USD]
dates = "LOCO_LONDON"
forward = { kind = "outright", averaging = "POINT", interpolation = "LINEAR", extrapolate = false, roll = "FORWARD_VOLATILITY" }
volatility = { kind = "delta", interpolation = "BILINEAR", extrapolate = false }
"""
SILVER_FORWARD = (
    (date(2026, 9, 16), 30.0),
    (date(2026, 10, 16), 30.1),
    (date(2026, 12, 16), 30.3),
    (date(2027, 3, 16), 30.6),
    (date(2027, 9, 16), 31.2),
)


class MarketRatesTest(unittest.TestCase):
    def setUp(self):
        self.connection = sqlite3.connect(":memory:")
        load_sample(self.connection)

    def tearDown(self):
        self.connection.close()

    def test_xau_usd_forward_is_the_gofo_outright(self):
        environment = self._rates("XAU", "USD")
        self.assertTrue(environment.validate_report().ok, environment.error_messages())
        settles = date(2026, 9, 18)
        discount = self._usd_discount()
        year_fraction = (VALUE - settles).days / 360.0
        outright = (
            XAU_SPOT
            * discount.get_discount_factor(settles)
            / discount.get_discount_factor(VALUE)
            / (1.0 + GOFO_DEC * year_fraction)
        )
        expected = outright * discount_factor(USD_CONTINUOUS_RATE, VALUE)
        self.assertAlmostEqual(self._forward_value(environment, VALUE), expected)

    def test_load_market_returns_the_curves_it_prices_with(self):
        loaded = load_market("XAU", SAMPLE_ANCHOR, "USD", self.connection)
        self.assertEqual(loaded.spec.market, "XAU")
        self.assertEqual(loaded.spec.currency, "USD")
        self.assertEqual(loaded.anchor, SAMPLE_ANCHOR)
        settles = date(2026, 9, 18)
        year_fraction = (VALUE - settles).days / 360.0
        outright = (
            XAU_SPOT
            * loaded.discount.get_discount_factor(settles)
            / loaded.discount.get_discount_factor(VALUE)
            / (1.0 + GOFO_DEC * year_fraction)
        )
        self.assertAlmostEqual(loaded.forward.get_forward(VALUE), outright)
        day = date(2027, 1, 15)
        discounted = loaded.forward.get_forward(day) * loaded.discount.get_discount_factor(day)
        self.assertAlmostEqual(self._forward_value(loaded.environment, day), discounted)
        again = rates_environment("XAU", SAMPLE_ANCHOR, "USD", self.connection)
        self.assertAlmostEqual(
            self._forward_value(again, day), self._forward_value(loaded.environment, day)
        )

    def test_xau_zar_forward_is_the_usd_outright_times_usdzar(self):
        usd = self._rates("XAU", "USD")
        zar = self._rates("XAU", "ZAR")
        self.assertTrue(zar.validate_report().ok, zar.error_messages())
        usd_forward = self._forward_value(usd, VALUE) / discount_factor(USD_CONTINUOUS_RATE, VALUE)
        usdzar = dict(USDZAR_FORWARD)[VALUE]
        expected = usd_forward * usdzar * discount_factor(ZAR_CONTINUOUS_RATE, VALUE)
        self.assertAlmostEqual(self._forward_value(zar, VALUE), expected)

    def test_xpt_usd_forward_is_the_stored_outright(self):
        environment = self._rates("xpt", "usd")
        self.assertTrue(environment.validate_report().ok, environment.error_messages())
        pillar = date(2027, 9, 16)
        outright = dict(XPT_FORWARD)[pillar]
        expected = outright * discount_factor(USD_CONTINUOUS_RATE, pillar)
        self.assertAlmostEqual(self._forward_value(environment, pillar), expected)

    def test_dated_brent_forward_is_the_contract_month(self):
        environment = self._rates("DATED_BRENT", "USD")
        self.assertTrue(environment.validate_report().ok, environment.error_messages())
        settlement = date(2026, 10, 16)
        expected = 75.0 * discount_factor(USD_CONTINUOUS_RATE, settlement)
        self.assertAlmostEqual(self._forward_value(environment, settlement), expected)
        with self.assertRaises(QuoteNotAvailable):
            self._rates("DATED_BRENT", "ZAR")

    def test_default_environment_cannot_drive_a_monte_carlo(self):
        environment = self._rates("XPT", "USD")
        report = self._evolver(environment, date(2027, 3, 16)).validate_report()
        self.assertIn(
            "PATH_GENERATOR_FORWARD_NOT_STOCHASTIC", [issue.code for issue in report.issues]
        )

    def test_every_quote_runs_a_monte_carlo_with_a_stochastic_roll(self):
        for market in MARKETS.values():
            for quote in market.quotes.values():
                if (quote.market, quote.currency) not in SAMPLE_QUOTES:
                    continue
                monthly = quote.forward.averaging is ForwardAveraging.CALENDAR_MONTH
                settlement = date(2027, 2, 16) if monthly else date(2027, 3, 16)
                rolls = [ForwardRoll.FORWARD_VOLATILITY]
                if not monthly:
                    rolls.append(ForwardRoll.SPOT_VOLATILITY)
                deterministic = rates_environment(
                    quote.market, SAMPLE_ANCHOR, quote.currency, self.connection
                )
                expected = self._forward_value(deterministic, settlement)
                for roll in rolls:
                    with self.subTest(market=quote.market, currency=quote.currency, roll=roll):
                        environment = rates_environment(
                            quote.market, SAMPLE_ANCHOR, quote.currency, self.connection, roll
                        )
                        self.assertAlmostEqual(self._forward_value(environment, settlement), expected)
                        evolver = self._evolver(environment, settlement)
                        report = evolver.validate_report()
                        self.assertTrue(report.ok, [issue.message for issue in report.issues])
                        forward = sjd.EuropeanForward(settlement, 0.0, 1.0, sjd.BuySell.BUY)
                        simulation = sjd.simulate(
                            sjd.Portfolio([forward]),
                            environment,
                            SIMULATION_DATES,
                            paths=20,
                            seed=3,
                        )
                        self.assertEqual(simulation.steps, len(SIMULATION_DATES))
                        self.assertTrue(np.isfinite(simulation.mtm).all())
                        self.assertAlmostEqual(simulation.mtm[0, 0], expected, places=6)

    def test_calendar_month_market_has_no_spot_volatility_roll(self):
        with self.assertRaises(ValueError):
            self._rates("DATED_BRENT", "USD", ForwardRoll.SPOT_VOLATILITY)

    def test_a_new_market_needs_only_a_catalogue_entry_and_pillars(self):
        catalogue = parse_catalogue(SILVER_CATALOGUE)
        connection = sqlite3.connect(":memory:")
        self.addCleanup(connection.close)
        initialize(connection, catalogue)
        connection.execute("PRAGMA foreign_keys = ON")
        connection.execute(
            "INSERT INTO rates_label (name, as_of, anchor_date) VALUES ('silver', '2026-09-16T00:00:00', ?)",
            (SAMPLE_ANCHOR.isoformat(),),
        )
        connection.executemany(
            """
            INSERT INTO forward_pillar (label_name, as_of, market, currency, pillar_date, value)
            VALUES ('silver', '2026-09-16T00:00:00', 'XAG', 'USD', ?, ?)
            """,
            [(day.isoformat(), value) for day, value in SILVER_FORWARD],
        )
        connection.executemany(
            """
            INSERT INTO discount_pillar (label_name, as_of, currency, pillar_date, discount_factor)
            VALUES ('silver', '2026-09-16T00:00:00', 'USD', ?, ?)
            """,
            [
                (day.isoformat(), discount_factor(USD_CONTINUOUS_RATE, day))
                for day, _value in SILVER_FORWARD
            ],
        )
        connection.executemany(
            """
            INSERT INTO volatility_pillar (
                label_name, as_of, market, currency, pillar_date, strike_dimension, volatility
            ) VALUES ('silver', '2026-09-16T00:00:00', 'XAG', 'USD', ?, ?, ?)
            """,
            [
                (day.isoformat(), delta, 0.25)
                for day, _value in SILVER_FORWARD
                for delta in (10.0, 50.0, 90.0)
            ],
        )
        connection.commit()

        environment = rates_environment(
            "XAG", SAMPLE_ANCHOR, "USD", connection, catalogue=catalogue
        )
        settlement = date(2026, 12, 16)
        expected = 30.3 * discount_factor(USD_CONTINUOUS_RATE, settlement)
        self.assertAlmostEqual(self._forward_value(environment, settlement), expected)
        report = self._evolver(environment, settlement).validate_report()
        self.assertTrue(report.ok, [issue.message for issue in report.issues])
        with self.assertRaises(QuoteNotAvailable):
            rates_environment("XAG", SAMPLE_ANCHOR, "USD", self.connection)

    def test_saving_and_opening_round_trips_a_quote(self):
        anchor = SAMPLE_ANCHOR
        dates, strikes, grid = volatility_grid(self.connection, anchor, "XPT", "USD")
        volatility = [
            (day, strike, grid[strike_index][date_index])
            for date_index, day in enumerate(dates)
            for strike_index, strike in enumerate(strikes)
        ]
        with tempfile.TemporaryDirectory() as folder:
            saved = Path(folder) / "pillars.db"
            save_market_pillars(
                saved,
                label="close",
                as_of=datetime(2026, 9, 16, 16, 0),
                anchor=anchor,
                market="XPT",
                currency="USD",
                discount=discount_pillars(self.connection, anchor, "USD"),
                forward=forward_pillars(self.connection, anchor, "XPT", "USD"),
                volatility=volatility,
            )
            opened = open_market(saved, "XPT", "USD", label="close")
        original = self._forward_value(self._rates("XPT", "USD"), VALUE)
        self.assertAlmostEqual(self._forward_value(opened.environment, VALUE), original)
        self.assertEqual(opened.spot_date(), spot_date(quote_spec("XPT", "USD"), anchor))
        self.assertEqual(opened.label.name, "close")

    def test_a_label_opens_the_newest_instance_and_keeps_one_anchor(self):
        anchor = SAMPLE_ANCHOR
        dates, strikes, grid = volatility_grid(self.connection, anchor, "XPT", "USD")
        volatility = [
            (day, strike, grid[strike_index][date_index])
            for date_index, day in enumerate(dates)
            for strike_index, strike in enumerate(strikes)
        ]
        discount = discount_pillars(self.connection, anchor, "USD")
        forward = forward_pillars(self.connection, anchor, "XPT", "USD")
        later = [(day, value + 1.0) for day, value in forward]
        with tempfile.TemporaryDirectory() as folder:
            saved = Path(folder) / "pillars.db"
            save_market_pillars(
                saved,
                label="close",
                as_of="2026-09-16T10:00:00",
                anchor=anchor,
                market="XPT",
                currency="USD",
                discount=discount,
                forward=forward,
                volatility=volatility,
            )
            save_market_pillars(
                saved,
                label="close",
                as_of=datetime(2026, 9, 16, 16, 0),
                anchor=anchor,
                market="XPT",
                currency="USD",
                discount=discount,
                forward=later,
                volatility=volatility,
            )
            newest = open_market(saved, "XPT", "USD", label="close")
            earlier = open_market(saved, "XPT", "USD", label="close", as_of="2026-09-16T10:00:00")
            with self.assertRaises(RatesLabelError):
                save_market_pillars(
                    saved,
                    label="close",
                    as_of="2026-09-16T16:00:00",
                    anchor=date(2026, 9, 17),
                    market="XPT",
                    currency="USD",
                    discount=discount,
                    forward=forward,
                    volatility=volatility,
                )
        self.assertEqual(newest.label.as_of, datetime(2026, 9, 16, 16, 0))
        self.assertAlmostEqual(newest.forward.get_forward(VALUE), earlier.forward.get_forward(VALUE) + 1.0)

    def test_a_cross_overlay_prices_like_the_shipped_zar_cross(self):
        overlay = parse_catalogue(
            """
            [markets.XAU.quotes.ZAR]
            dates = "LOCO_LONDON"
            forward = { kind = "cross", base_market = "XAU", base_currency = "USD", fx_market = "USDZAR", fx_currency = "ZAR" }
            volatility = { kind = "delta" }
            """,
            inherit=CATALOGUE.source,
        )
        connection = sqlite3.connect(":memory:")
        self.addCleanup(connection.close)
        load_sample(connection, overlay)
        crossed = rates_environment("XAU", SAMPLE_ANCHOR, "ZAR", connection, catalogue=overlay)
        self.assertAlmostEqual(
            self._forward_value(crossed, VALUE),
            self._forward_value(self._rates("XAU", "ZAR"), VALUE),
        )

    def test_gofo_rate_grows_spot_by_the_simple_rate(self):
        catalogue = parse_catalogue(
            """
            [calendars.LONDON_METALS]
            name = "UnitedKingdom.Metals"

            [dates.LOCO_LONDON]
            kind = "point"
            calendar = "LONDON_METALS"
            spot_lag_business_days = 2
            expiry_before_value_business_days = 2

            [discount.USD]
            interpolation = "LINEAR"
            extrapolate = false

            [markets.XAG]
            summary = "Silver quoted as a GOFO swap rate."

            [markets.XAG.quotes.USD]
            dates = "LOCO_LONDON"
            forward = { kind = "gofo_rate" }
            volatility = { kind = "flat" }
            """
        )
        anchor = SAMPLE_ANCHOR
        spot_day = spot_date(quote_spec("XAU", "USD"), anchor)
        value = date(2026, 12, 16)
        later = date(2027, 3, 16)
        rate = 0.03
        loaded = build_market(
            "XAG",
            anchor,
            "USD",
            spot=100.0,
            discount=[(anchor, 1.0), (spot_day, 0.99), (value, 0.97), (later, 0.95)],
            forward=[(value, rate), (later, rate)],
            volatility=[(anchor, 0.0, 0.2), (value, 0.0, 0.2), (later, 0.0, 0.2)],
            catalogue=catalogue,
        )
        tau = (value - spot_day).days / 360.0
        self.assertAlmostEqual(loaded.forward.get_forward(value), 100.0 * (1.0 + rate * tau))
        rolled = loaded.with_roll(ForwardRoll.FORWARD_VOLATILITY)
        self.assertAlmostEqual(rolled.forward.get_forward(value), loaded.forward.get_forward(value))

    def test_an_unknown_calendar_is_refused_before_any_pillar_is_read(self):
        catalogue = parse_catalogue(SILVER_CATALOGUE.replace('"UnitedKingdom.Metals"', '"Narnia"'))
        with self.assertRaisesRegex(ValueError, "Narnia"):
            rates_environment("XAG", SAMPLE_ANCHOR, "USD", self.connection, catalogue=catalogue)

    def _rates(self, market, currency, roll=None):
        return rates_environment(market, SAMPLE_ANCHOR, currency, self.connection, roll)

    def _evolver(self, environment, settlement):
        forward = sjd.EuropeanForward(settlement, 0.0, 1.0, sjd.BuySell.BUY)
        portfolio = sjd.Portfolio([forward])
        ready = portfolio.validate_report(environment)
        self.assertTrue(ready.ok, [issue.message for issue in ready.issues])
        generator = sjd.SingleFactorEconomicScenarioGenerator(environment)
        generator.set_random_seed(3)
        return sjd.PortfolioEvolver(portfolio, generator, SIMULATION_DATES)

    def _usd_discount(self):
        pillars = discount_pillars(self.connection, SAMPLE_ANCHOR, "USD")
        return sjd.DiscountCurve(
            SAMPLE_ANCHOR,
            [day for day, _factor in pillars],
            [factor for _day, factor in pillars],
        )

    def _forward_value(self, environment, settlement):
        forward = sjd.EuropeanForward(settlement, 0.0, 1.0, sjd.BuySell.BUY)
        report = forward.validate_report(environment)
        self.assertTrue(report.ok, forward.error_messages())
        return forward.get_value(environment)


class CalendarTest(unittest.TestCase):
    LONDON = CalendarSpec("UnitedKingdom")

    def test_london_skips_christmas_and_the_boxing_day_substitute(self):
        self.assertEqual(business_days_after(date(2026, 12, 23), 2, self.LONDON), date(2026, 12, 29))
        self.assertEqual(
            calendar(self.LONDON).holidays(date(2026, 12, 1), date(2026, 12, 31)),
            [date(2026, 12, 25), date(2026, 12, 28)],
        )

    def test_usdzar_is_closed_when_either_country_is(self):
        usdzar = quote_spec("USDZAR", "ZAR")
        joint = calendar(usdzar.dates.calendar)
        self.assertEqual(joint.components, ("UnitedStates", "SouthAfrica"))
        self.assertTrue(sjd.Calendar("SouthAfrica").is_business_day(date(2026, 7, 3)))
        self.assertFalse(joint.is_business_day(date(2026, 7, 3)))
        self.assertEqual(spot_date(usdzar, date(2026, 7, 2)), date(2026, 7, 7))
        self.assertEqual(spot_date(usdzar, date(2026, 12, 23)), date(2026, 12, 28))

    def test_option_expiry_counts_back_from_the_value_date(self):
        xau = quote_spec("XAU", "USD")
        self.assertEqual(spot_date(xau, SAMPLE_ANCHOR), date(2026, 9, 18))
        self.assertEqual(option_expiry(xau, date(2026, 12, 29)), date(2026, 12, 23))

    def test_calendar_month_quotes_have_no_spot_lag(self):
        with self.assertRaises(QuoteNotAvailable):
            spot_date(quote_spec("DATED_BRENT", "USD"), SAMPLE_ANCHOR)
        with self.assertRaises(QuoteNotAvailable):
            month_settlement(quote_spec("XAU", "USD"), SAMPLE_ANCHOR)

    def test_dated_brent_settles_five_business_days_after_the_last_pricing_date(self):
        brent = quote_spec("DATED_BRENT", "USD")
        december = pricing_dates(brent, date(2026, 12, 10))
        self.assertEqual(len(december), 21)
        self.assertNotIn(date(2026, 12, 25), december)
        self.assertNotIn(date(2026, 12, 28), december)
        self.assertEqual(december[-1], date(2026, 12, 31))
        self.assertEqual(month_settlement(brent, date(2026, 12, 10)), date(2027, 1, 8))

    def test_calendar_binding(self):
        self.assertIn("UnitedKingdom.Metals", sjd.Calendar.names())
        with self.assertRaisesRegex(ValueError, "Unknown calendar"):
            sjd.Calendar("Narnia")
        london = sjd.Calendar("UnitedKingdom")
        self.assertEqual(london.name, "UnitedKingdom")
        self.assertEqual(london.components, ())
        self.assertEqual(london.adjust(date(2026, 12, 25)), date(2026, 12, 29))
        self.assertEqual(
            london.adjust(date(2026, 12, 25), sjd.BusinessDayConvention.PRECEDING), date(2026, 12, 24)
        )
        self.assertEqual(
            london.advance(date(2026, 11, 30), 1, sjd.TimeUnit.MONTHS, sjd.BusinessDayConvention.MODIFIED_FOLLOWING),
            date(2026, 12, 30),
        )
        self.assertEqual(london.business_days_between(date(2026, 12, 21), date(2026, 12, 31)), 6)


if __name__ == "__main__":
    unittest.main()
