"""The classes bound in step 19, including pickle and text."""

import math
import pickle
import unittest
from datetime import date

import sjd

ANCHOR = date(2026, 1, 2)
SPOT = date(2026, 1, 6)
SETTLEMENT = date(2027, 1, 4)
DATES = [ANCHOR, SPOT, SETTLEMENT]


def discount(factors):
    return sjd.DiscountCurve(ANCHOR, DATES, factors)


def round_trip(value):
    return pickle.loads(pickle.dumps(value))


class CashflowTest(unittest.TestCase):
    def test_value_is_the_signed_amount_discounted_on_the_curve(self):
        self.assertIs(sjd.Cashflow, sjd.FixedCashflow)
        environment = sjd.RatesEnvironment(ANCHOR)
        environment.set_drs(discount([1.0, 0.99, 0.97]))
        bought = sjd.FixedCashflow(SETTLEMENT, 1000.0, sjd.BuySell.BUY)
        sold = sjd.FixedCashflow(SETTLEMENT, 1000.0, sjd.BuySell.SELL)

        self.assertAlmostEqual(bought.get_value(environment), 970.0)
        self.assertAlmostEqual(sold.get_value(environment), -970.0)
        self.assertAlmostEqual(bought.get_payoff(0.0), 1000.0)
        self.assertAlmostEqual(sold.get_payoff(50.0), -1000.0)
        self.assertEqual(bought.maturity_date(), SETTLEMENT)

        later = sjd.RatesEnvironment(date(2027, 2, 1))
        later.set_drs(sjd.DiscountCurve(later.anchor_date(), [later.anchor_date()], [1.0]))
        self.assertEqual(bought.get_value(later), 0.0)


class FxForwardTest(unittest.TestCase):
    def test_spot_date_is_moved_to_the_anchor(self):
        domestic = discount([1.0, 0.99, 0.95])
        foreign = discount([1.0, 0.98, 0.90])
        spot = 1100.0
        spot_at_anchor = spot * 0.99 / 0.98
        expected = spot_at_anchor * 0.90 / 0.95

        for curve in (
            sjd.ForwardCurveFX(ANCHOR, spot, SPOT, domestic, foreign),
            sjd.ForwardCurveDeterministicFX(ANCHOR, spot, SPOT, domestic, foreign),
        ):
            with self.subTest(curve=type(curve).__name__):
                self.assertTrue(curve.is_ok(), curve.error_messages())
                self.assertAlmostEqual(curve.get_forward(ANCHOR), spot_at_anchor)
                self.assertAlmostEqual(curve.get_forward(SETTLEMENT), expected)

        already = sjd.ForwardCurveFX(ANCHOR, spot_at_anchor, domestic, foreign)
        self.assertAlmostEqual(already.get_forward(SETTLEMENT), expected)

        volatility = sjd.VolatilityCurve(ANCHOR, DATES, [0.1, 0.1, 0.1])
        stochastic = sjd.ForwardCurveStochasticFX(ANCHOR, spot, SPOT, domestic, foreign, volatility)
        self.assertTrue(stochastic.is_ok(), stochastic.error_messages())
        self.assertAlmostEqual(stochastic.get_forward(SETTLEMENT), expected)

        environment = sjd.RatesEnvironment(ANCHOR)
        environment.set_drs(domestic)
        environment.set_foreign_drs(foreign)
        environment.set_frs(stochastic)
        self.assertIs(environment.foreign_drs(), foreign)
        self.assertAlmostEqual(environment.foreign_drs().get_discount_factor(SETTLEMENT), 0.90)

    def test_environment_reads_back_every_curve(self):
        environment = sjd.RatesEnvironment(ANCHOR)
        self.assertIsNone(environment.frs())
        self.assertIsNone(environment.drs())
        self.assertIsNone(environment.vrs())
        self.assertIsNone(environment.fixing_rs())
        forward = sjd.ForwardCurve(ANCHOR, DATES, [100.0, 101.0, 104.0])
        domestic = discount([1.0, 0.99, 0.95])
        volatility = sjd.VolatilityCurve(ANCHOR, DATES, [0.1, 0.1, 0.1])
        fixings = sjd.DailyFixingCurve([ANCHOR], [100.0])
        environment.set_frs(forward)
        environment.set_drs(domestic)
        environment.set_vrs(volatility)
        environment.set_fixing_rs(fixings)
        self.assertIs(environment.frs(), forward)
        self.assertIs(environment.drs(), domestic)
        self.assertIs(environment.vrs(), volatility)
        self.assertIs(environment.fixing_rs(), fixings)
        self.assertAlmostEqual(environment.frs().get_forward(SETTLEMENT), 104.0)

    def test_contract_terms_are_readable(self):
        option = sjd.EuropeanOption(
            SPOT, SETTLEMENT, 100.0, sjd.PutCall.PUT, 10.0, SETTLEMENT, 1.5, sjd.BuySell.SELL
        )
        self.assertEqual(option.strike, 100.0)
        self.assertEqual(option.type, sjd.PutCall.PUT)
        self.assertEqual(option.premium_amount, 1.5)
        self.assertEqual(option.premium_payment_date(), SETTLEMENT)
        forward = sjd.EuropeanForward(SETTLEMENT, 99.0, 10.0, sjd.BuySell.BUY)
        self.assertEqual(forward.strike, 99.0)
        asian = sjd.AsianOption(
            [SPOT], SETTLEMENT, 80.0, sjd.PutCall.CALL, 2.0, SETTLEMENT, 0.0, sjd.BuySell.BUY
        )
        self.assertEqual(asian.strike, 80.0)
        self.assertEqual(asian.type, sjd.PutCall.CALL)
        self.assertEqual(asian.premium_amount, 0.0)
        self.assertEqual(sjd.AsianForward([SPOT], SETTLEMENT, 81.0, 2.0, sjd.BuySell.BUY).strike, 81.0)


class StressAndInsuranceTest(unittest.TestCase):
    def environment(self):
        environment = sjd.RatesEnvironment(ANCHOR)
        environment.set_drs(discount([1.0, 0.99, 0.95]))
        environment.set_frs(sjd.ForwardCurve(ANCHOR, DATES, [100.0, 101.0, 104.0]))
        return environment

    def test_stressed_values_are_a_dict(self):
        environment = self.environment()
        forward = sjd.EuropeanForward(SETTLEMENT, 100.0, 10.0, sjd.BuySell.BUY)
        values = sjd.ContractStressor(forward, environment).stressed_values()
        self.assertIsInstance(values, dict)
        self.assertIn("Base", values)
        self.assertTrue(all(isinstance(name, str) and math.isfinite(value) for name, value in values.items()))

    def test_constant_and_scheduled_sums_insured(self):
        curve = discount([1.0, 0.99, 0.95])
        constant = sjd.SimpleInsurance(ANCHOR, SETTLEMENT, 1000.0, curve, curve)
        self.assertTrue(constant.is_ok(), constant.error_messages())
        self.assertEqual(constant.sum_insured(SPOT), 1000.0)
        self.assertEqual(constant.sum_insured(date(2028, 1, 1)), 0.0)
        payment = constant.insurance_payments([SETTLEMENT], [5000.0])
        self.assertEqual(len(payment), 1)
        self.assertAlmostEqual(payment[0], 1000.0 * (1.0 / 0.95 - 1.0))

        scheduled = sjd.SimpleInsuranceWithTermStructure(
            ANCHOR, SETTLEMENT, [SPOT, SETTLEMENT], [400.0, 250.0], curve, curve
        )
        self.assertTrue(scheduled.is_ok(), scheduled.error_messages())
        self.assertEqual(scheduled.sum_insured(SPOT), 400.0)
        self.assertEqual(scheduled.sum_insured(SETTLEMENT), 250.0)
        self.assertEqual(len(scheduled.insurance_payments([SPOT, SETTLEMENT], [10.0, 10.0])), 2)


class VolatilityAndDiscountTest(unittest.TestCase):
    def test_moneyness_outside_the_grid_reads_the_wing(self):
        moneyness = [-0.2, 0.0, 0.2]
        grid = [[0.30, 0.30], [0.20, 0.20], [0.35, 0.35]]
        surface = sjd.VolatilitySurfaceMoneyness(
            ANCHOR, [ANCHOR, SETTLEMENT], moneyness, grid, sjd.SurfaceInterpolator.BILINEAR
        )
        self.assertTrue(surface.is_ok(), surface.error_messages())
        self.assertAlmostEqual(surface.get_volatility(SETTLEMENT, 100.0, 100.0), 0.20)
        self.assertAlmostEqual(surface.get_volatility(SETTLEMENT, 70.0, 100.0), 0.30)
        self.assertAlmostEqual(surface.get_volatility(SETTLEMENT, 140.0, 100.0), 0.35)

    def test_discount_factor_and_rate_round_trip_and_a_bump_lowers_the_factor(self):
        convention = sjd.InterestRateConvention(sjd.Compounding.CONTINUOUS, sjd.Frequency.ANNUAL)
        start = date(2026, 1, 1)
        end = date(2027, 1, 1)
        discount_factor = convention.rate_to_df(0.05, start, end)
        self.assertAlmostEqual(convention.df_to_rate(discount_factor, start, end), 0.05)

        curve = discount([1.0, 0.99, 0.95])
        bumped = curve.parallel_bump(0.01, convention)
        self.assertLess(bumped.get_discount_factor(SETTLEMENT), curve.get_discount_factor(SETTLEMENT))
        self.assertAlmostEqual(curve.get_discount_factor(SETTLEMENT), 0.95)


class AsianFormulaTest(unittest.TestCase):
    def test_one_live_forward_matches_black76(self):
        call = sjd.AverageRateCall([100.0], [], 100.0, [0.2], 0.97)
        put = sjd.AverageRatePut([100.0], [], 100.0, [0.2], 0.97)
        self.assertAlmostEqual(call.premium(), sjd.Black76Call(100.0, 100.0, 0.2, 0.97).premium())
        self.assertAlmostEqual(put.premium(), sjd.Black76Put(100.0, 100.0, 0.2, 0.97).premium())


class MarginAndCapitalTest(unittest.TestCase):
    def test_initial_margin_is_stored_and_does_not_change_the_call(self):
        margin = sjd.SimpleMargin(ANCHOR, True, 1000.0, True, 1000.0, 500.0, 100.0)
        margin.set_margin_parameters(
            ANCHOR, True, 1000.0, True, 250.0, True, 1000.0, False, 0.0, 500.0, 100.0
        )
        self.assertTrue(margin.is_ok(), margin.error_messages())
        self.assertTrue(margin.requires_initial_margin())
        self.assertEqual(margin.initial_margin(), 250.0)
        self.assertEqual(margin.get_margin_balance(), 0.0)
        self.assertEqual(margin.get_margin_call(10_000.0), 9_000.0)

        margin.set_margin_parameters(
            ANCHOR, True, 1000.0, True, -1.0, True, 1000.0, False, 0.0, 500.0, 100.0
        )
        self.assertFalse(margin.is_ok())

    def test_capital_returns_the_breakdown(self):
        environment = sjd.RatesEnvironment(ANCHOR)
        environment.set_drs(discount([1.0, 0.99, 0.97]))
        environment.set_frs(sjd.ForwardCurve(ANCHOR, DATES, [100.0, 100.0, 100.0]))
        forward = sjd.EuropeanForward(SETTLEMENT, 90.0, 10.0, sjd.BuySell.BUY)
        portfolio = sjd.Portfolio([forward])
        credit = sjd.CounterpartyCredit(1.0, 0.08)
        result = sjd.CurrentExposureMethod(portfolio, sjd.reg_cap_factors("commodity"), credit).capital(
            portfolio, None, environment
        )
        self.assertGreater(result.ead, 0.0)
        self.assertAlmostEqual(result.rwa, result.ead * credit.risk_weight())
        self.assertGreater(result.capital, 0.0)
        self.assertEqual(result.net_gross_ratio, 1.0)
        self.assertEqual(len(result.components), 1)
        component = result.components[0]
        self.assertEqual(component["settlement"], SETTLEMENT)
        self.assertEqual(component["volume"], 10.0)
        self.assertIn("capped", component)
        self.assertIn("mark_to_market", component)
        self.assertIn("add_on", component)
        self.assertIn("CapitalResult", repr(result))


class PickleTest(unittest.TestCase):
    def test_books_and_curves_round_trip(self):
        domestic = discount([1.0, 0.99, 0.95])
        foreign = discount([1.0, 0.98, 0.90])
        volatility = sjd.VolatilityCurve(ANCHOR, [SPOT, SETTLEMENT], [0.2, 0.25])
        fixing = sjd.DailyFixingCurve([ANCHOR, SPOT], [99.0, 100.0])
        surface = sjd.VolatilitySurfaceMoneyness(
            ANCHOR,
            [ANCHOR, SETTLEMENT],
            [-0.2, 0.0, 0.2],
            [[0.30, 0.31], [0.20, 0.21], [0.35, 0.36]],
            sjd.SurfaceInterpolator.BILINEAR,
        )
        forward = sjd.ForwardCurve(ANCHOR, [SPOT, SETTLEMENT], [101.0, 104.0], volatility=volatility)
        fx = sjd.ForwardCurveFX(ANCHOR, 1100.0, SPOT, domestic, foreign)
        cashflow = sjd.FixedCashflow(SETTLEMENT, 1000.0, sjd.BuySell.BUY)
        option = sjd.EuropeanOption(
            SETTLEMENT, SETTLEMENT, 100.0, sjd.PutCall.CALL, 5.0, ANCHOR, 1.5, sjd.BuySell.BUY
        )
        european = sjd.EuropeanForward(SPOT, SETTLEMENT, 102.0, 8.0, sjd.BuySell.SELL)
        asian_forward = sjd.AsianForward([ANCHOR, SPOT], SETTLEMENT, 101.0, 4.0, sjd.BuySell.BUY)
        asian_option = sjd.AsianOption(
            [ANCHOR, SPOT], SETTLEMENT, 101.0, sjd.PutCall.PUT, 4.0, ANCHOR, 0.0, sjd.BuySell.BUY
        )
        margin = sjd.SimpleMargin(ANCHOR, True, 1000.0, True, 1000.0, 500.0, 100.0)
        margin.set_margin_parameters(
            ANCHOR, True, 1000.0, True, 250.0, True, 1000.0, True, 80.0, 500.0, 100.0
        )
        margin.set_margin_balance(40.0)
        portfolio = sjd.Portfolio([cashflow, option, european, asian_forward, asian_option], margin)
        environment = sjd.RatesEnvironment(ANCHOR)
        environment.set_frs(forward)
        environment.set_drs(domestic)
        environment.set_foreign_drs(foreign)
        environment.set_vrs(surface)
        environment.set_fixing_rs(fixing)

        restored_cashflow = round_trip(cashflow)
        self.assertIsInstance(restored_cashflow, sjd.FixedCashflow)
        environment.set_frs(fx)
        self.assertAlmostEqual(restored_cashflow.get_value(environment), cashflow.get_value(environment))

        restored_option = round_trip(option)
        self.assertEqual(restored_option.volume, option.volume)
        self.assertEqual(restored_option.buy_sell, option.buy_sell)
        restored_forward = round_trip(european)
        self.assertEqual(restored_forward.maturity_date(), SPOT)
        self.assertEqual(restored_forward.settlement_date(), SETTLEMENT)
        restored_asian = round_trip(asian_option)
        self.assertEqual(list(restored_asian.averaging_dates()), list(asian_option.averaging_dates()))

        restored_discount = round_trip(domestic)
        self.assertAlmostEqual(restored_discount.get_discount_factor(SETTLEMENT), 0.95)
        restored_forward_curve = round_trip(forward)
        self.assertAlmostEqual(restored_forward_curve.get_forward(SETTLEMENT), 104.0)
        restored_fx = round_trip(fx)
        self.assertIsInstance(restored_fx, sjd.ForwardCurveFX)
        self.assertAlmostEqual(restored_fx.get_forward(SETTLEMENT), fx.get_forward(SETTLEMENT))
        stochastic = sjd.ForwardCurveStochasticFX(ANCHOR, 1100.0, SPOT, domestic, foreign, volatility)
        restored_stochastic = round_trip(stochastic)
        self.assertIsInstance(restored_stochastic, sjd.ForwardCurveStochasticFX)
        self.assertAlmostEqual(restored_stochastic.get_forward(SETTLEMENT), stochastic.get_forward(SETTLEMENT))
        fx_environment = sjd.RatesEnvironment(ANCHOR)
        fx_environment.set_frs(fx)
        fx_environment.set_drs(domestic)
        fx_environment.set_foreign_drs(foreign)
        priced = sjd.EuropeanForward(SETTLEMENT, 1000.0, 1.0, sjd.BuySell.BUY)
        restored_fx_environment = round_trip(fx_environment)
        self.assertAlmostEqual(
            priced.get_value(restored_fx_environment), priced.get_value(fx_environment)
        )
        restored_volatility = round_trip(volatility)
        self.assertAlmostEqual(restored_volatility.get_volatility(SETTLEMENT), 0.25)
        restored_fixing = round_trip(fixing)
        self.assertAlmostEqual(restored_fixing.get_rate(SPOT), 100.0)
        restored_surface = round_trip(surface)
        self.assertAlmostEqual(
            restored_surface.get_volatility(SETTLEMENT, 100.0, 100.0),
            surface.get_volatility(SETTLEMENT, 100.0, 100.0),
        )

        restored_portfolio = round_trip(portfolio)
        self.assertEqual(len(restored_portfolio.contracts()), 5)
        self.assertIsInstance(restored_portfolio.contracts()[0], sjd.FixedCashflow)
        self.assertIsInstance(restored_portfolio.contracts()[2], sjd.EuropeanForward)
        self.assertEqual(restored_portfolio.get_margin_balance(), 40.0)
        self.assertEqual(restored_portfolio.contracts()[0].volume, 1000.0)

        environment.set_frs(forward)
        book = sjd.Portfolio([european])
        restored_environment = round_trip(environment)
        self.assertEqual(restored_environment.anchor_date(), ANCHOR)
        self.assertAlmostEqual(restored_environment.foreign_drs().get_discount_factor(SPOT), 0.98)
        self.assertAlmostEqual(book.get_value(restored_environment), book.get_value(environment))
        held = sjd.RatesEnvironment(ANCHOR)
        held.set_frs(forward)
        held.set_drs(domestic)
        held.set_vrs(volatility)
        held.set_fixing_rs(fixing)
        held.set_foreign_drs(foreign)
        restored_held = round_trip(held)
        self.assertAlmostEqual(restored_held.foreign_drs().get_discount_factor(SETTLEMENT), 0.90)

        for value in (
            cashflow,
            option,
            forward,
            fx,
            domestic,
            volatility,
            fixing,
            surface,
            portfolio,
            environment,
            margin,
            sjd.Black76Call(100.0, 100.0, 0.2, 0.97),
            sjd.InterestRateConvention(sjd.Compounding.CONTINUOUS, sjd.Frequency.ANNUAL),
        ):
            text = repr(value)
            self.assertTrue(text)
            self.assertIn(type(value).__name__, text)
