import unittest
from datetime import date

import sjd
from sjd import examples


class ValidationReportTest(unittest.TestCase):
    def test_empty_rates_environment_report_is_additive(self):
        environment = sjd.RatesEnvironment(date(2026, 9, 16))

        snapshot = environment.validation_report()
        self.assertTrue(snapshot.ok)
        self.assertEqual(snapshot.issues, ())

        report = environment.validate_report()
        self.assertFalse(report.ok)
        self.assertEqual(len(report.issues), 1)
        self.assertEqual(report.issues[0].code, "RATES_ENVIRONMENT_NO_RATES")
        self.assertEqual(report.issues[0].path, ("rates_environment",))
        self.assertEqual(report.issues[0].message, "No rates set")
        self.assertEqual(
            environment.error_messages(),
            "RatesEnvironment: No rates set",
        )

    def test_european_forward_report_preserves_legacy_api(self):
        anchor = date(2026, 9, 16)
        settlement = date(2026, 10, 16)
        environment = sjd.RatesEnvironment(anchor)
        forward = sjd.EuropeanForward(
            settlement=settlement,
            strike=-1.0,
            volume=1_000_000.0,
            buy_sell=sjd.BuySell.BUY,
        )

        self.assertTrue(forward.validation_report().ok)

        report = forward.validate_report(environment)
        self.assertFalse(report.ok)
        self.assertEqual(
            [issue.code for issue in report.issues],
            [
                "CONTRACT_STRIKE_NEGATIVE",
                "RATES_ENVIRONMENT_NO_RATES",
                "MARKET_DISCOUNT_RATE_SOURCE_MISSING",
                "MARKET_FORWARD_RATE_SOURCE_MISSING",
            ],
        )
        self.assertEqual(
            report.issues[0].path,
            ("european_forward", "strike"),
        )
        self.assertEqual(forward.is_ok(), False)

        legacy_before = forward.error_messages()
        self.assertIsNone(forward.validate(environment))
        self.assertEqual(forward.error_messages(), legacy_before)

    def test_nested_invalid_discount_curve_has_component_path(self):
        anchor = date(2026, 9, 16)
        invalid_curve = sjd.DiscountCurve(
            anchor_date=anchor,
            observation_dates=[anchor, date(2027, 9, 16)],
            discount_factors=[1.0],
            allow_extrapolation=False,
        )
        environment = sjd.RatesEnvironment(anchor)
        environment.set_drs(invalid_curve)

        report = environment.validate_report()
        self.assertFalse(report.ok)
        self.assertEqual(len(report.issues), 1)
        self.assertEqual(report.issues[0].code, "DISCOUNT_CURVE_DIMENSION_MISMATCH")
        self.assertEqual(
            report.issues[0].path,
            (
                "rates_environment",
                "discount_rate_source",
                "discount_curve",
                "inputs",
            ),
        )

    def test_report_collections_are_tuples(self):
        report = sjd.RatesEnvironment(date(2026, 9, 16)).validate_report()
        self.assertIsInstance(report.issues, tuple)
        self.assertIsInstance(report.issues[0].path, tuple)
        self.assertEqual(report.issues[0].severity, sjd.ValidationSeverity.ERROR)


class PortfolioValidationTest(unittest.TestCase):
    def test_option_book_reports_each_contract_path(self):
        anchor = date(2026, 9, 16)
        settlement = date(2026, 12, 16)
        european = sjd.EuropeanOption(
            maturity=settlement,
            settlement=settlement,
            strike=17.6,
            type=sjd.PutCall.CALL,
            volume=1_000_000.0,
            premium_payment_date=anchor,
            premium_amount=0.0,
            buy_sell=sjd.BuySell.BUY,
        )
        asian = sjd.AsianOption(
            averaging_dates=[anchor, settlement],
            settlement=settlement,
            strike=17.4,
            type=sjd.PutCall.PUT,
            volume=500_000.0,
            premium_payment_date=anchor,
            premium_amount=0.0,
            buy_sell=sjd.BuySell.BUY,
        )
        portfolio = sjd.Portfolio([european, asian])
        report = portfolio.validate_report(sjd.RatesEnvironment(anchor))

        self.assertFalse(report.ok)
        codes = {issue.path[1]: issue.code for issue in report.issues if "volatility_rate_source" in issue.path or "fixing_rate_source" in issue.path}
        self.assertIn("contracts[0]", codes)
        self.assertEqual(codes["contracts[0]"], "MARKET_VOLATILITY_RATE_SOURCE_MISSING")
        self.assertTrue(any(
            issue.path[:3] == ("portfolio", "contracts[0]", "european_option")
            for issue in report.issues
        ))
        self.assertTrue(any(
            issue.code == "MARKET_FIXING_RATE_SOURCE_MISSING"
            and issue.path[:3] == ("portfolio", "contracts[1]", "asian_option")
            for issue in report.issues
        ))

    def test_evolver_refuses_a_deterministic_forward(self):
        anchor = date(2026, 9, 16)
        settlement = date(2026, 12, 16)
        dates = [anchor, settlement]
        environment = sjd.RatesEnvironment(anchor)
        environment.set_drs(sjd.DiscountCurve(anchor, dates, [1.0, 0.98]))
        environment.set_frs(sjd.ForwardCurve(anchor, dates, [17.5, 17.8]))
        environment.set_vrs(sjd.VolatilityCurve(anchor, dates, [0.2, 0.2]))
        option = sjd.EuropeanOption(
            maturity=settlement,
            settlement=settlement,
            strike=17.6,
            type=sjd.PutCall.CALL,
            volume=1.0,
            premium_payment_date=anchor,
            premium_amount=0.0,
            buy_sell=sjd.BuySell.BUY,
        )
        portfolio = sjd.Portfolio([option])
        ready = portfolio.validate_report(environment)
        self.assertTrue(ready.ok, [issue.message for issue in ready.issues])

        generator = sjd.SingleFactorEconomicScenarioGenerator(environment)
        evolver = sjd.PortfolioEvolver(portfolio, generator, [anchor, settlement])
        report = evolver.validate_report()
        self.assertFalse(report.ok)
        self.assertTrue(any(
            issue.code == "PATH_GENERATOR_FORWARD_NOT_STOCHASTIC"
            for issue in report.issues
        ))
        self.assertFalse(evolver.is_ok())

    def test_evolver_accepts_regulatory_capital(self):
        anchor = date(2026, 9, 16)
        settlement = date(2026, 12, 16)
        dates = [anchor, settlement]
        environment = sjd.RatesEnvironment(anchor)
        environment.set_drs(sjd.DiscountCurve(anchor, dates, [1.0, 0.98]))
        environment.set_frs(sjd.ForwardCurve(anchor, dates, [17.5, 17.8]))
        environment.set_vrs(sjd.VolatilityCurve(anchor, dates, [0.2, 0.2]))
        option = sjd.EuropeanOption(
            maturity=settlement,
            settlement=settlement,
            strike=17.6,
            type=sjd.PutCall.CALL,
            volume=1.0,
            premium_payment_date=anchor,
            premium_amount=0.0,
            buy_sell=sjd.BuySell.BUY,
        )
        portfolio = sjd.PortfolioB2CEM(
            [option],
            sjd.reg_cap_factors("commodity"),
            sjd.RegCapCreditParameters(1.5, 0.1),
        )
        ready = portfolio.validate_report(environment)
        self.assertTrue(ready.ok, [issue.message for issue in ready.issues])
        capital = portfolio.get_regulatory_capital(environment)
        self.assertGreaterEqual(capital, 0.0)

        generator = sjd.SingleFactorEconomicScenarioGenerator(environment)
        evolver = sjd.PortfolioEvolver(
            portfolio,
            generator,
            [anchor, settlement],
            portfolio.regulatory_capital(),
        )
        report = evolver.validate_report()
        self.assertTrue(any(
            issue.code == "PATH_GENERATOR_FORWARD_NOT_STOCHASTIC"
            for issue in report.issues
        ))


class PortfolioAnalysisTest(unittest.TestCase):
    def setUp(self):
        self.market = examples.sample_market()
        self.book = examples.sample_contracts(self.market["forward"], self.market)
        self.environment = self.market["environment"]

    def test_every_contract_shares_the_contract_api(self):
        contracts = self.book["contracts"]
        for contract in contracts:
            self.assertIsInstance(contract, sjd.SingleInstrumentContract)
            self.assertIsInstance(contract, sjd.Contract)
            self.assertTrue(contract.validate_report(self.environment).ok)
        portfolio = sjd.Portfolio(contracts)
        self.assertIsInstance(portfolio, sjd.Contract)
        self.assertEqual(
            [type(contract) for contract in portfolio.contracts()],
            [type(contract) for contract in contracts],
        )
        self.assertIs(portfolio.contracts()[0], contracts[0])
        self.assertEqual(self.book["forward"].volume, self.book["volume"])
        self.assertEqual(self.book["call"].buy_sell, sjd.BuySell.SELL)
        self.assertTrue(sjd.Portfolio(contracts).validate_report().ok)

    def test_a_portfolio_refuses_what_is_not_a_single_instrument(self):
        with self.assertRaisesRegex(TypeError, "single instruments.*Portfolio"):
            sjd.Portfolio([sjd.Portfolio(self.book["contracts"])])
        with self.assertRaisesRegex(TypeError, "str"):
            sjd.Portfolio(["forward"])

    def test_portfolio_value_matches_the_sum_of_the_legs(self):
        portfolio = sjd.Portfolio(self.book["contracts"])
        report = portfolio.validate_report(self.environment)
        self.assertTrue(report.ok, [issue.message for issue in report.issues])
        leg_sum = sum(contract.get_value(self.environment) for contract in self.book["contracts"])
        self.assertAlmostEqual(portfolio.get_value(self.environment), leg_sum, places=8)
        self.assertGreater(portfolio.get_value(self.environment), 0.0)

    def test_asian_option_can_be_priced(self):
        anchor = self.market["anchor"]
        settlement = self.market["collar_settlement"]
        option = sjd.AsianOption(
            averaging_dates=self.market["averaging_dates"],
            settlement=self.market["asian_settlement"],
            strike=self.book["asian_strike"],
            type=sjd.PutCall.CALL,
            volume=100_000.0,
            premium_payment_date=anchor,
            premium_amount=0.0,
            buy_sell=sjd.BuySell.BUY,
        )
        option.validate(self.environment)
        self.assertTrue(option.is_ok(), option.error_messages())
        self.assertGreater(option.get_value(self.environment), 0.0)
        self.assertNotEqual(settlement, anchor)

    def test_simple_margin_applies_threshold_transfer_and_rounding(self):
        margin = sjd.SimpleMargin(
            self.market["anchor"],
            True,
            1000.0,
            True,
            1000.0,
            500.0,
            100.0,
        )
        self.assertTrue(margin.is_ok())
        self.assertEqual(margin.get_margin_call(10_000.0), 9_000.0)
        self.assertEqual(margin.get_margin_call(1_400.0), 0.0)
        self.assertEqual(margin.get_margin_call(-10_000.0), -9_000.0)

    def test_posted_margin_reduces_capital_when_the_book_is_an_asset(self):
        factors = sjd.reg_cap_factors("commodity")
        credit = examples.credit_parameters()
        contracts = self.book["contracts"]
        bare = sjd.PortfolioB2CEM(contracts, factors, credit, False)
        margin = sjd.SimpleMargin(
            self.market["anchor"],
            True,
            50_000.0,
            True,
            50_000.0,
            10_000.0,
            1_000.0,
        )
        margined = sjd.PortfolioB2CEM(
            examples.sample_contracts(self.market["forward"], self.market)["contracts"],
            factors,
            credit,
            False,
            margin,
        )
        self.assertFalse(bare.is_margined())
        self.assertTrue(margined.is_margined())
        ready = margined.validate_report(self.environment)
        self.assertTrue(ready.ok, [issue.message for issue in ready.issues])
        bare.validate_report(self.environment)
        mtm = margined.get_value(self.environment)
        self.assertGreater(mtm, 0.0)
        self.assertAlmostEqual(margined.get_margin_call(self.environment), margin.get_margin_call(mtm))
        uncollateralised = margined.get_regulatory_capital(self.environment)
        self.assertAlmostEqual(uncollateralised, bare.get_regulatory_capital(self.environment))
        margined.set_margin_balance(mtm)
        self.assertLess(margined.get_regulatory_capital(self.environment), uncollateralised)

    def test_commodity_capital_exceeds_fx_and_netting_does_not_increase_it(self):
        credit = examples.credit_parameters()
        collar = [self.book["put"], self.book["call"]]
        separate = sjd.PortfolioB2CEM(collar, sjd.reg_cap_factors("commodity"), credit, False)
        netted = sjd.PortfolioB2CEM(
            examples.sample_contracts(self.market["forward"], self.market)["contracts"][:2],
            sjd.reg_cap_factors("commodity"),
            credit,
            True,
        )
        fx_book = sjd.PortfolioB2CEM(
            examples.sample_contracts(self.market["forward"], self.market)["contracts"][:2],
            sjd.reg_cap_factors("fx"),
            credit,
            True,
        )
        for portfolio in (separate, netted, fx_book):
            report = portfolio.validate_report(self.environment)
            self.assertTrue(report.ok, [issue.message for issue in report.issues])
        separate_capital = separate.get_regulatory_capital(self.environment)
        netted_capital = netted.get_regulatory_capital(self.environment)
        fx_capital = fx_book.get_regulatory_capital(self.environment)
        self.assertGreater(separate_capital, 0.0)
        self.assertLessEqual(netted_capital, separate_capital)
        self.assertGreater(netted_capital, fx_capital)

    def test_stochastic_forward_exposes_spot_and_monte_carlo_detail(self):
        market = examples.sample_market(stochastic=True)
        book = examples.sample_contracts(market["forward"], market)
        credit = examples.credit_parameters()
        portfolio = sjd.PortfolioB2CEM(
            book["contracts"],
            sjd.reg_cap_factors("commodity"),
            credit,
            True,
        )
        ready = portfolio.validate_report(market["environment"])
        self.assertTrue(ready.ok, [issue.message for issue in ready.issues])
        generator = sjd.SingleFactorEconomicScenarioGenerator(market["environment"])
        evolver = sjd.PortfolioEvolver(
            portfolio,
            generator,
            market["simulation_dates"],
            portfolio.regulatory_capital(),
        )
        report = evolver.validate_report()
        self.assertTrue(report.ok, [issue.message for issue in report.issues])
        spot_path = [market["forward"].get_forward(day) for day in market["simulation_dates"]]
        detail = evolver.get_spot_path_detail(spot_path)
        self.assertEqual(list(detail["headings"]), ["Spot Path", "MtM", "EE", "NEE", "EE with HP", "NEE with HP", "Reg Cap"])
        self.assertEqual(len(detail["rows"]), len(market["simulation_dates"]))
        self.assertAlmostEqual(detail["rows"][0][1], portfolio.get_value(market["environment"]), places=6)

        margin = sjd.SimpleMargin(market["anchor"], True, 50_000.0, True, 50_000.0, 10_000.0, 1_000.0)
        margined = sjd.PortfolioB2CEM(
            examples.sample_contracts(market["forward"], market)["contracts"],
            sjd.reg_cap_factors("commodity"),
            credit,
            True,
            margin,
        )
        margined.validate_report(market["environment"])
        margined_evolver = sjd.PortfolioEvolver(
            margined,
            sjd.SingleFactorEconomicScenarioGenerator(market["environment"]),
            market["simulation_dates"],
            margined.regulatory_capital(),
        )
        margined_report = margined_evolver.validate_report()
        self.assertTrue(margined_report.ok, [issue.message for issue in margined_report.issues])
        margined_detail = margined_evolver.get_spot_path_detail(spot_path)
        self.assertIn("Margin", list(margined_detail["headings"]))

        evolver.run_monte_carlo(20)
        monte_carlo = evolver.get_monte_carlo_detail()
        self.assertIn("Ave MtM", list(monte_carlo["headings"]))
        self.assertIn("PFE", list(monte_carlo["headings"]))
        self.assertEqual(len(monte_carlo["rows"]), len(market["simulation_dates"]))
        self.assertGreater(len(monte_carlo["rows"][0]), 2)

    def test_seeded_monte_carlo_repeats(self):
        market = examples.sample_market(stochastic=True)
        book = examples.sample_contracts(market["forward"], market)
        portfolio = sjd.Portfolio(book["contracts"])
        portfolio.validate_report(market["environment"])
        generator = sjd.SingleFactorEconomicScenarioGenerator(market["environment"])
        evolver = sjd.PortfolioEvolver(portfolio, generator, market["simulation_dates"])
        report = evolver.validate_report()
        self.assertTrue(report.ok, [issue.message for issue in report.issues])

        runs = []
        for _ in range(2):
            generator.set_random_seed(11)
            evolver.run_monte_carlo(21)
            runs.append(evolver.get_monte_carlo_detail()["rows"])
        self.assertEqual(runs[0], runs[1])


if __name__ == "__main__":
    unittest.main()
