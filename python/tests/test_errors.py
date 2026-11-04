"""Python exception types and the validation report text."""

import unittest
from datetime import date

import sjd


class ExceptionSurfaceTest(unittest.TestCase):
    def test_configuration_error_is_a_value_error(self):
        with self.assertRaises(sjd.ConfigurationError) as caught:
            sjd.Calendar("Narnia")
        error = caught.exception
        self.assertIsInstance(error, ValueError)
        self.assertIsInstance(error, sjd.SjdError)
        self.assertEqual(error.code, "CONFIGURATION")
        self.assertIn("Narnia", str(error))

        with self.assertRaises(ValueError):
            sjd.Black76Call(0.0, 100.0, 0.2, 1.0)

    def test_validation_error_carries_the_report(self):
        environment = sjd.RatesEnvironment(date(2026, 9, 16))
        with self.assertRaises(sjd.ValidationError) as caught:
            sjd.require_valid(environment)
        error = caught.exception
        self.assertIsInstance(error, ValueError)
        self.assertIsInstance(error, sjd.SjdError)
        self.assertEqual(error.code, "VALIDATION")
        self.assertIs(error.report.ok, False)
        self.assertEqual(
            str(error.report),
            "rates_environment: RATES_ENVIRONMENT_NO_RATES: No rates set",
        )
        self.assertEqual(len(str(error.report).splitlines()), len(error.report.issues))

    def test_report_string_has_one_line_per_issue(self):
        anchor = date(2026, 9, 16)
        environment = sjd.RatesEnvironment(anchor)
        forward = sjd.EuropeanForward(date(2026, 10, 16), -1.0, 1.0, sjd.BuySell.BUY)
        report = forward.validate(environment)
        self.assertGreater(len(report.issues), 1)
        lines = str(report).splitlines()
        self.assertEqual(len(lines), len(report.issues))
        self.assertTrue(all(": " in line for line in lines))
        self.assertIn("CONTRACT_STRIKE_NEGATIVE", lines[0])
        self.assertEqual(str(report.issues[0]), lines[0])

        described = report.to_dict()
        self.assertIs(described["ok"], False)
        self.assertEqual(described["issues"][0]["code"], "CONTRACT_STRIKE_NEGATIVE")
        self.assertEqual(described["issues"][0]["severity"], "ERROR")
        self.assertIn("european_forward", described["issues"][0]["path"])
        html = report._repr_html_()
        self.assertIn("<table>", html)
        self.assertIn("CONTRACT_STRIKE_NEGATIVE", html)
        self.assertIn("ValidationReport", repr(report))

        with self.assertRaises(sjd.ValidationError) as caught:
            report.raise_if_invalid()
        self.assertEqual(str(caught.exception.report), str(report))

    def test_a_valid_report_does_not_raise(self):
        environment = sjd.RatesEnvironment(date(2026, 9, 16))
        self.assertTrue(environment.report.ok)
        environment.report.raise_if_invalid()
        self.assertIn("issues=0", repr(environment.report))

    def test_numerical_error_is_an_arithmetic_error(self):
        error = sjd.NumericalError("solver failed")
        self.assertIsInstance(error, ArithmeticError)
        self.assertIsInstance(error, sjd.SjdError)
        self.assertNotIsInstance(error, ValueError)
        self.assertEqual(error.code, "NUMERICAL")

    def test_sjd_error_keeps_its_code(self):
        error = sjd.SjdError("Requirement failed: ready", "REQUIRE")
        self.assertIsInstance(error, RuntimeError)
        self.assertNotIsInstance(error, ValueError)
        self.assertEqual(error.code, "REQUIRE")
        self.assertEqual(str(error), "Requirement failed: ready")
