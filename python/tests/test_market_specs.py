"""Conventions for the four default markets. No rate objects are built."""

import unittest

from sjd.markets.specs import (
    CATALOGUE_PATH,
    MARKETS,
    CalendarMonthDates,
    CatalogueError,
    CalendarMonthMoneynessVol,
    CurveInterpolation,
    DeltaVol,
    DiscountCurveSpec,
    ForwardAveraging,
    ForwardRoll,
    GofoForward,
    MetalTimesUsdZarForward,
    OutrightForward,
    PointDates,
    QuoteNotAvailable,
    SurfaceInterpolation,
    load_catalogue,
    market_spec,
    parse_catalogue,
    quote_spec,
)

POINT_PREAMBLE = """
[calendars.LONDON]
name = "UnitedKingdom"

[dates.SPOT]
kind = "point"
calendar = "LONDON"
spot_lag_business_days = 2
expiry_before_value_business_days = 2

[discount.USD]
interpolation = "LINEAR"
extrapolate = false
"""


def one_quote(forward, volatility='{ kind = "delta" }', dates="SPOT", currency="USD"):
    return POINT_PREAMBLE + f"""
[markets.XAG.quotes.{currency}]
dates = "{dates}"
forward = {forward}
volatility = {volatility}
"""


class MarketSpecTest(unittest.TestCase):
    def test_known_markets(self):
        self.assertEqual(
            set(market_spec(name).market for name in ("XAU", "XPT", "DATED_BRENT", "USDZAR")),
            {"XAU", "XPT", "DATED_BRENT", "USDZAR"},
        )

    def test_xau_usd_is_gofo_and_delta(self):
        quote = quote_spec("XAU", "USD")

        self.assertIsInstance(quote.forward, GofoForward)
        self.assertIs(quote.forward.averaging, ForwardAveraging.POINT)
        self.assertEqual(quote.forward.interpolation, CurveInterpolation.LINEAR)
        self.assertFalse(quote.forward.extrapolate)
        self.assertIsInstance(quote.volatility, DeltaVol)
        self.assertEqual(quote.volatility.interpolation, SurfaceInterpolation.BILINEAR)
        self.assertFalse(quote.volatility.extrapolate)
        self.assertEqual(quote.discount_currency, "USD")
        self._assert_usd_discount(quote.discount)
        self.assertTrue(quote.stores_forward)
        self._assert_loco_london(quote.dates)

    def test_xpt_usd_is_an_outright(self):
        quote = quote_spec("xpt", "usd")

        self.assertIsInstance(quote.forward, OutrightForward)
        self.assertIs(quote.forward.averaging, ForwardAveraging.POINT)
        self.assertEqual(quote.forward.interpolation, CurveInterpolation.LINEAR)
        self.assertFalse(quote.forward.extrapolate)
        self.assertIsInstance(quote.volatility, DeltaVol)
        self.assertEqual(quote.volatility.interpolation, SurfaceInterpolation.BILINEAR)
        self.assertFalse(quote.volatility.extrapolate)
        self._assert_usd_discount(quote.discount)
        self._assert_loco_london(quote.dates)

    def test_zar_metal_forward_is_the_usd_metal_times_usdzar(self):
        for metal in ("XAU", "XPT"):
            quote = quote_spec(metal, "ZAR")
            usd = quote_spec(metal, "USD")
            fx = quote_spec("USDZAR", "ZAR")

            self.assertIsInstance(quote.forward, MetalTimesUsdZarForward)
            self.assertEqual(quote.forward.metal_market, metal)
            self.assertEqual(quote.forward.metal_currency, "USD")
            self.assertEqual(quote.forward.fx_market, "USDZAR")
            self.assertIs(quote.forward.averaging, ForwardAveraging.POINT)
            self.assertFalse(quote.stores_forward)
            self.assertNotIn("interpolation", quote.forward.__dataclass_fields__)
            self.assertNotIn("extrapolate", quote.forward.__dataclass_fields__)
            self.assertTrue(usd.stores_forward)
            self.assertTrue(fx.stores_forward)
            self.assertEqual(quote.discount_currency, "ZAR")
            self.assertIs(quote.discount, fx.discount)
            self.assertIsNot(quote.discount, usd.discount)
            self.assertFalse(quote.discount.extrapolate)
            self.assertIsInstance(quote.volatility, DeltaVol)
            self.assertEqual(quote.volatility.interpolation, SurfaceInterpolation.BILINEAR)
            self._assert_loco_london(quote.dates)

    def test_dated_brent_is_a_calendar_month_market_in_usd_only(self):
        quote = quote_spec("DATED_BRENT", "USD")

        self.assertIsInstance(quote.forward, OutrightForward)
        self.assertIs(quote.forward.averaging, ForwardAveraging.CALENDAR_MONTH)
        self.assertEqual(quote.forward.interpolation, CurveInterpolation.LINEAR)
        self.assertFalse(quote.forward.extrapolate)
        self.assertIsInstance(quote.volatility, CalendarMonthMoneynessVol)
        self.assertEqual(quote.volatility.interpolation, SurfaceInterpolation.BILINEAR)
        self.assertTrue(quote.volatility.extrapolate)
        self._assert_usd_discount(quote.discount)
        self.assertIs(quote.discount, quote_spec("XAU", "USD").discount)
        self.assertIsInstance(quote.dates, CalendarMonthDates)
        self.assertEqual(quote.dates.settlement_lag_business_days, 5)
        self.assertEqual(quote.dates.publication_calendar.name, "UnitedKingdom")

        with self.assertRaises(QuoteNotAvailable):
            quote_spec("DATED_BRENT", "ZAR")

    def test_usdzar_is_an_outright_on_the_joint_calendar(self):
        quote = quote_spec("USDZAR", "ZAR")

        self.assertIsInstance(quote.forward, OutrightForward)
        self.assertIs(quote.forward.averaging, ForwardAveraging.POINT)
        self.assertEqual(quote.forward.interpolation, CurveInterpolation.LINEAR)
        self.assertFalse(quote.forward.extrapolate)
        self.assertIsInstance(quote.volatility, DeltaVol)
        self.assertEqual(quote.volatility.interpolation, SurfaceInterpolation.BILINEAR)
        self.assertFalse(quote.volatility.extrapolate)
        self.assertEqual(quote.discount_currency, "ZAR")
        self.assertIs(quote.discount, quote_spec("XAU", "ZAR").discount)
        self.assertEqual(quote.discount.interpolation, CurveInterpolation.LINEAR)
        self.assertFalse(quote.discount.extrapolate)
        self.assertIsInstance(quote.dates, PointDates)
        self.assertEqual(quote.dates.spot_lag_business_days, 2)
        self.assertEqual(quote.dates.expiry_before_value_business_days, 2)
        self.assertEqual(
            quote.dates.calendar.components, ("UnitedStates", "SouthAfrica")
        )

        with self.assertRaises(QuoteNotAvailable):
            quote_spec("USDZAR", "USD")

    def test_every_default_quote_is_deterministic(self):
        for market in MARKETS.values():
            for quote in market.quotes.values():
                with self.subTest(market=quote.market, currency=quote.currency):
                    self.assertIs(quote.forward.roll, ForwardRoll.DETERMINISTIC)

    def test_roll_is_chosen_per_forward(self):
        monthly = ForwardAveraging.CALENDAR_MONTH
        stochastic = OutrightForward(averaging=monthly, roll=ForwardRoll.FORWARD_VOLATILITY)
        self.assertIs(stochastic.roll, ForwardRoll.FORWARD_VOLATILITY)
        self.assertIs(
            GofoForward(roll=ForwardRoll.SPOT_VOLATILITY).roll, ForwardRoll.SPOT_VOLATILITY
        )
        with self.assertRaises(ValueError):
            OutrightForward(averaging=monthly, roll=ForwardRoll.SPOT_VOLATILITY)
        with self.assertRaises(TypeError):
            GofoForward(roll="FORWARD_VOLATILITY")
        with self.assertRaises(TypeError):
            MetalTimesUsdZarForward("XAU", roll="DETERMINISTIC")

    def test_interpolation_is_chosen_per_curve(self):
        cubic = OutrightForward(
            averaging=ForwardAveraging.POINT,
            interpolation=CurveInterpolation.CUBIC,
        )
        bicubic = DeltaVol(interpolation=SurfaceInterpolation.BICUBIC)

        self.assertEqual(cubic.interpolation, CurveInterpolation.CUBIC)
        self.assertEqual(bicubic.interpolation, SurfaceInterpolation.BICUBIC)
        self.assertEqual(
            quote_spec("XPT", "USD").forward.interpolation,
            CurveInterpolation.LINEAR,
        )
        with self.assertRaises(TypeError):
            DeltaVol(interpolation="BILINEAR")
        with self.assertRaises(TypeError):
            OutrightForward(averaging=ForwardAveraging.POINT, extrapolate="no")
        with self.assertRaises(TypeError):
            OutrightForward(averaging="point")

    def _assert_usd_discount(self, discount):
        self.assertIsInstance(discount, DiscountCurveSpec)
        self.assertEqual(discount.interpolation, CurveInterpolation.LINEAR)
        self.assertFalse(discount.extrapolate)

    def _assert_loco_london(self, dates):
        self.assertIsInstance(dates, PointDates)
        self.assertEqual(dates.calendar.name, "UnitedKingdom.Metals")
        self.assertEqual(dates.spot_lag_business_days, 2)
        self.assertEqual(dates.expiry_before_value_business_days, 2)


class CatalogueTest(unittest.TestCase):
    def test_the_shipped_file_is_the_default_catalogue(self):
        self.assertEqual(load_catalogue(CATALOGUE_PATH).markets, MARKETS)

    def test_a_quote_is_parsed_into_the_spec_types(self):
        catalogue = parse_catalogue(
            one_quote(
                '{ kind = "outright", averaging = "POINT", interpolation = "CUBIC", '
                'roll = "SPOT_VOLATILITY" }',
                '{ kind = "delta", interpolation = "BICUBIC" }',
            )
        )
        quote = catalogue.quote_spec("xag", "usd")
        self.assertEqual(
            quote.forward,
            OutrightForward(
                averaging=ForwardAveraging.POINT,
                interpolation=CurveInterpolation.CUBIC,
                roll=ForwardRoll.SPOT_VOLATILITY,
            ),
        )
        self.assertEqual(quote.volatility, DeltaVol(interpolation=SurfaceInterpolation.BICUBIC))
        self.assertIs(quote.discount, catalogue.discount_curves["USD"])
        self.assertEqual(quote.dates.calendar.name, "UnitedKingdom")

    def test_mistakes_name_the_place_in_the_file(self):
        cases = {
            "interpolation must be one of LINEAR, CUBIC": one_quote(
                '{ kind = "outright", averaging = "POINT", interpolation = "linear" }'
            ),
            "kind must be one of": one_quote('{ kind = "swap" }'),
            "unexpected keyword argument 'extrapolation'": one_quote(
                '{ kind = "outright", averaging = "POINT", extrapolation = false }'
            ),
            "names 'MISSING', which is not defined": one_quote(
                '{ kind = "outright", averaging = "POINT" }', dates="MISSING"
            ),
            "which has no [discount.EUR]": one_quote(
                '{ kind = "outright", averaging = "POINT" }', currency="EUR"
            ),
            "averages by CALENDAR_MONTH but its dates are PointDates": one_quote(
                '{ kind = "outright", averaging = "CALENDAR_MONTH" }',
                '{ kind = "calendar_month_moneyness" }',
            ),
            "no spot-volatility roll": one_quote(
                '{ kind = "outright", averaging = "CALENDAR_MONTH", roll = "SPOT_VOLATILITY" }'
            ),
            "crosses a missing quote": one_quote('{ kind = "metal_times_usdzar", metal_market = "XAU" }'),
        }
        for message, text in cases.items():
            with self.subTest(message=message):
                with self.assertRaises(CatalogueError) as caught:
                    parse_catalogue(text)
                self.assertIn(message, str(caught.exception))
                self.assertIn("markets.XAG.quotes", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
