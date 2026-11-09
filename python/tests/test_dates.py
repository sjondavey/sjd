"""Dates accept datetime, numpy.datetime64, and pandas.Timestamp."""

import unittest
from datetime import date, datetime

import numpy as np
import pytest

import sjd

DAY = date(2026, 9, 16)
SATURDAY = date(2026, 9, 19)


class DateInputTest(unittest.TestCase):
    def setUp(self):
        self.calendar = sjd.Calendar("WeekendsOnly")

    def test_datetime_and_numpy_agree(self):
        self.assertTrue(self.calendar.is_business_day(DAY))
        self.assertTrue(self.calendar.is_business_day(datetime(2026, 9, 16, 15, 30)))
        self.assertTrue(self.calendar.is_business_day(np.datetime64("2026-09-16")))
        self.assertTrue(self.calendar.is_business_day(np.datetime64("2026-09-16T15:30")))
        self.assertTrue(self.calendar.is_business_day(np.datetime64("1969-12-31")))
        self.assertFalse(self.calendar.is_business_day(np.datetime64(SATURDAY.isoformat())))

    def test_numpy_nat_is_rejected(self):
        with self.assertRaises(ValueError):
            self.calendar.is_business_day(np.datetime64("NaT"))

    def test_a_string_is_not_a_date(self):
        with self.assertRaises(TypeError):
            self.calendar.is_business_day("2026-09-16")

    def test_pandas_timestamp(self):
        pandas = pytest.importorskip("pandas")
        self.assertTrue(self.calendar.is_business_day(pandas.Timestamp(DAY)))
        with self.assertRaises(ValueError):
            self.calendar.is_business_day(pandas.NaT)
