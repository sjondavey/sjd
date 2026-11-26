"""TOML books, report tables, and the ``python -m sjd run`` command."""

import importlib.util
import tempfile
import unittest
from pathlib import Path

import sjd
from sjd import book, examples

SAMPLE_BOOK = Path(__file__).resolve().parents[1] / "sjd" / "books" / "sample.toml"
_HAVE_PANDAS = importlib.util.find_spec("pandas") is not None
_HAVE_OPENPYXL = importlib.util.find_spec("openpyxl") is not None


class BookTest(unittest.TestCase):
    def setUp(self):
        self.market = examples.sample_market()
        self.environment = self.market["environment"]

    def test_sample_book_matches_the_example_contracts(self):
        loaded = book.load_book(SAMPLE_BOOK, self.market)
        reference = examples.sample_contracts(self.market["forward"], self.market)
        self.assertEqual(loaded.name, "sample")
        self.assertEqual([trade.kind for trade in loaded.trades], ["collar", "european_forward", "asian_forward"])
        priced = loaded.portfolio()
        report = priced.validate_report(self.environment)
        self.assertTrue(report.ok, [issue.message for issue in report.issues])
        self.assertAlmostEqual(
            priced.get_value(self.environment),
            sjd.Portfolio(reference["contracts"]).get_value(self.environment),
        )

    def test_absolute_cashflow_and_a_rejected_kind(self):
        settlement = self.market["forward_settlement"].isoformat()
        text = f"""
[[trades]]
kind = "cashflow"
settlement = {settlement}
volume = 250
buy_sell = "SELL"
"""
        loaded = book.parse_book(text)
        cashflow = loaded.contracts()[0]
        self.assertIsInstance(cashflow, sjd.FixedCashflow)
        self.assertEqual(cashflow.buy_sell, sjd.BuySell.SELL)
        self.assertAlmostEqual(cashflow.get_payoff(0.0), -250.0)
        with self.assertRaises(book.BookError):
            book.parse_book('[[trades]]\nkind = "swap"\n')

    def test_margin_table_is_attached_to_the_portfolio(self):
        settlement = self.market["forward_settlement"].isoformat()
        text = f"""
[[trades]]
kind = "european_forward"
settlement = {settlement}
strike = 18
volume = 10
buy_sell = "BUY"

[margin]
use_my_threshold = true
my_threshold = 1000
use_cpty_threshold = true
cpty_threshold = 1000
minimum_transfer_amount = 100
rounding = 10
"""
        loaded = book.parse_book(text, self.market)
        self.assertTrue(loaded.portfolio().is_margined())
        self.assertEqual(loaded.margin.get_margin_call(10_000.0), 9_000.0)

    def test_client_selects_the_banks_side(self):
        maturity = self.market["collar_maturity"].isoformat()
        settlement = self.market["collar_settlement"].isoformat()

        def collar(client_line):
            text = f"""
[[trades]]
kind = "collar"
maturity = {maturity}
settlement = {settlement}
put_strike = 17
call_strike = 18
volume = 10
{client_line}
"""
            return book.parse_book(text).contracts()

        producer_put, producer_call = collar('client = "producer"')
        omitted_put, omitted_call = collar("")
        consumer_put, consumer_call = collar('client = "consumer"')
        self.assertEqual(producer_put.buy_sell, sjd.BuySell.SELL)
        self.assertEqual(producer_call.buy_sell, sjd.BuySell.BUY)
        self.assertEqual(omitted_put.buy_sell, producer_put.buy_sell)
        self.assertEqual(omitted_call.buy_sell, producer_call.buy_sell)
        self.assertEqual(consumer_put.buy_sell, sjd.BuySell.BUY)
        self.assertEqual(consumer_call.buy_sell, sjd.BuySell.SELL)
        with self.assertRaisesRegex(book.BookError, "producer or consumer"):
            collar('client = "dealer"')


@unittest.skipUnless(_HAVE_PANDAS, "pandas is not installed")
class ReportTest(unittest.TestCase):
    def test_prices_and_excel(self):
        from sjd import reports

        market = examples.sample_market(stochastic=True)
        loaded = book.load_book(SAMPLE_BOOK, market)
        portfolio = loaded.portfolio()
        environment = market["environment"]
        sjd.require_valid(portfolio, environment)
        frame = reports.prices(loaded.contracts(), environment)
        self.assertEqual(list(frame["kind"]), ["EuropeanOption", "EuropeanOption", "EuropeanForward", "AsianForward"])
        self.assertAlmostEqual(frame["value"].sum(), portfolio.get_value(environment))
        self.assertEqual(list(frame["put_call"][:2]), ["PUT", "CALL"])
        self.assertTrue((frame["strike"] > 0).all())

        simulation = sjd.simulate(
            portfolio, environment, market["simulation_dates"], paths=4, seed=2, antithetic=False
        )
        profile = reports.exposure_profile(simulation)
        self.assertEqual(len(profile), len(market["simulation_dates"]))
        if not _HAVE_OPENPYXL:
            return
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "report.xlsx"
            reports.write_excel(path, {"prices": frame, "exposure": profile})
            from openpyxl import load_workbook

            saved = load_workbook(path)
            self.assertEqual(saved.sheetnames, ["prices", "exposure"])


@unittest.skipUnless(_HAVE_PANDAS, "pandas is not installed")
class CommandTest(unittest.TestCase):
    def test_run_writes_the_report_folder(self):
        from sjd.__main__ import main

        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "report"
            status = main(
                ["run", str(SAMPLE_BOOK), "--market", "sample", "--paths", "4", "--seed", "3", "--output", str(output)]
            )
            self.assertEqual(status, 0)
            self.assertTrue((output / "prices.csv").is_file())
            self.assertTrue((output / "exposure.csv").is_file())
            self.assertTrue((output / "costs.csv").is_file())
            self.assertTrue((output / "capital.csv").is_file())
            if _HAVE_OPENPYXL:
                self.assertTrue((output / "report.xlsx").is_file())
        self.assertEqual(main(["run", str(SAMPLE_BOOK), "--market", "other"]), 2)
