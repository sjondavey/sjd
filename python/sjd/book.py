"""A TOML book of trades, loaded into ``sjd`` contracts.

The file uses the same date and table style as the market catalogue. A trade
is one instrument or one strategy. ``schedule`` takes dates from the market
instead of writing them in the file. A moneyness is resolved with the market
forward when the book is loaded.
"""

from __future__ import annotations

import sys
from datetime import date, datetime
from pathlib import Path

if sys.version_info >= (3, 11):
    import tomllib
else:
    import tomli as tomllib

import sjd
from sjd.strategy import (
    Client,
    CollarStrategy,
    ParticipatingForwardStrategy,
    SeagullStrategy,
    StripStrategy,
)

_BOOK_FIELDS = {"name", "volume"}
_MARGIN_FIELDS = {
    "use_my_threshold",
    "my_threshold",
    "use_cpty_threshold",
    "cpty_threshold",
    "minimum_transfer_amount",
    "rounding",
}
_TRADE_FIELDS = {
    "cashflow": {"kind", "schedule", "settlement", "volume", "buy_sell"},
    "european_forward": {
        "kind", "schedule", "settlement", "strike", "strike_moneyness", "volume", "buy_sell",
    },
    "european_option": {
        "kind", "schedule", "maturity", "settlement", "strike", "strike_moneyness", "type",
        "volume", "premium_payment_date", "premium_amount", "buy_sell",
    },
    "asian_forward": {
        "kind", "schedule", "averaging_dates", "settlement", "strike", "strike_moneyness",
        "volume", "buy_sell",
    },
    "asian_option": {
        "kind", "schedule", "averaging_dates", "settlement", "strike", "strike_moneyness",
        "type", "volume", "premium_payment_date", "premium_amount", "buy_sell",
    },
    "collar": {
        "kind", "schedule", "maturity", "settlement", "put_strike", "put_moneyness",
        "call_strike", "call_moneyness", "volume", "premium_payment_date", "put_premium",
        "call_premium", "client",
    },
    "strip": {"kind", "settlements", "strikes", "strike", "strike_moneyness", "volume", "buy_sell"},
    "seagull": {
        "kind", "schedule", "maturity", "settlement", "floor_strike", "floor_moneyness",
        "put_strike", "put_moneyness", "call_strike", "call_moneyness", "volume",
        "premium_payment_date", "floor_premium", "put_premium", "call_premium", "client",
    },
    "participating_forward": {
        "kind", "schedule", "maturity", "settlement", "put_strike", "put_moneyness",
        "call_strike", "call_moneyness", "participation", "volume", "premium_payment_date",
        "put_premium", "call_premium", "client",
    },
}


class BookError(ValueError):
    """The book file does not describe a loadable set of trades."""


class Book:
    """Trades loaded from TOML, each already turned into contracts."""

    def __init__(self, name, trades, margin=None):
        self.name = name
        self.trades = tuple(trades)
        self.margin = margin

    def contracts(self) -> list:
        """Every instrument, in trade order."""
        instruments = []
        for trade in self.trades:
            instruments.extend(trade.contracts)
        return instruments

    def portfolio(self, margin=None):
        """A portfolio of the book. The file's margin is used when ``margin`` is omitted."""
        held = self.margin if margin is None else margin
        instruments = self.contracts()
        if held is None:
            return sjd.Portfolio(instruments)
        return sjd.Portfolio(instruments, held)


class LoadedTrade:
    """One ``[[trades]]`` entry and the contracts it built."""

    def __init__(self, kind, contracts):
        self.kind = kind
        self.contracts = tuple(contracts)


def load_book(path, market=None) -> Book:
    """Read a TOML book from ``path``."""
    text = Path(path).read_text(encoding="utf-8")
    return parse_book(text, market)


def parse_book(text, market=None) -> Book:
    """Read a TOML book from ``text``.

    ``market`` is the sample market dict or a ``LoadedMarket``. A loaded market
    supplies the forward, the discount curve, and the anchor. Named schedules
    still need the sample market's date keys.
    """
    market = _market_view(market)
    try:
        data = tomllib.loads(text)
    except tomllib.TOMLDecodeError as error:
        raise BookError(f"book is not valid TOML: {error}") from error
    if not isinstance(data, dict):
        raise BookError("book must be a table")
    header = data.get("book", {})
    if not isinstance(header, dict):
        raise BookError("[book] must be a table")
    _unknown(header, _BOOK_FIELDS, "[book]")
    trades = data.get("trades")
    if not isinstance(trades, list) or len(trades) == 0:
        raise BookError("a book needs at least one trade")
    default_volume = header.get("volume")
    loaded = tuple(
        _trade(entry, index, default_volume, market) for index, entry in enumerate(trades)
    )
    margin_table = data.get("margin")
    margin = None
    if margin_table is not None:
        if market is None:
            raise BookError("[margin] needs a market")
        if not isinstance(margin_table, dict):
            raise BookError("[margin] must be a table")
        _unknown(margin_table, _MARGIN_FIELDS, "[margin]")
        margin = sjd.SimpleMargin(
            market["anchor"],
            bool(margin_table.get("use_my_threshold", False)),
            float(margin_table.get("my_threshold", 0.0)),
            bool(margin_table.get("use_cpty_threshold", False)),
            float(margin_table.get("cpty_threshold", 0.0)),
            float(margin_table.get("minimum_transfer_amount", 0.0)),
            float(margin_table.get("rounding", 1.0)),
        )
    return Book(str(header.get("name", "")), loaded, margin)


def _trade(entry, index, default_volume, market) -> LoadedTrade:
    where = f"trades[{index}]"
    if not isinstance(entry, dict):
        raise BookError(f"{where} must be a table")
    kind = entry.get("kind")
    if kind not in _TRADE_FIELDS:
        raise BookError(f"{where}.kind is not a known trade")
    _unknown(entry, _TRADE_FIELDS[kind], where)
    volume = _volume(entry, default_volume, where)
    if kind == "cashflow":
        settlement = _date(_need(entry, "settlement", where, market), f"{where}.settlement", market)
        return LoadedTrade(kind, [sjd.FixedCashflow(settlement, volume, _side(entry, where))])
    if kind == "european_forward":
        settlement = _date(_need(entry, "settlement", where, market), f"{where}.settlement", market)
        strike = _level(entry, where, "strike", "strike_moneyness", market, settlement)
        return LoadedTrade(kind, [sjd.EuropeanForward(settlement, strike, volume, _side(entry, where))])
    if kind == "european_option":
        return LoadedTrade(kind, [_european(entry, where, volume, market)])
    if kind == "asian_forward":
        return LoadedTrade(kind, [_asian(entry, where, volume, market, option=False)])
    if kind == "asian_option":
        return LoadedTrade(kind, [_asian(entry, where, volume, market, option=True)])
    if kind == "collar":
        return LoadedTrade(kind, _collar(entry, where, volume, market).contracts())
    if kind == "strip":
        return LoadedTrade(kind, _strip(entry, where, volume, market).contracts())
    if kind == "seagull":
        return LoadedTrade(kind, _seagull(entry, where, volume, market).contracts())
    return LoadedTrade(kind, _participating(entry, where, volume, market).contracts())


def _collar(entry, where, volume, market) -> CollarStrategy:
    maturity = _date(_need(entry, "maturity", where, market), f"{where}.maturity", market)
    settlement = _date(_need(entry, "settlement", where, market), f"{where}.settlement", market)
    forward = _forward(market, settlement, where)
    client = _client(entry, where)
    try:
        return CollarStrategy(
            maturity=maturity,
            settlement=settlement,
            put_strike=_wing(entry, where, "put_strike", "put_moneyness", forward),
            call_strike=_wing(entry, where, "call_strike", "call_moneyness", forward),
            volume=volume,
            premium_payment_date=_optional_date(entry, "premium_payment_date", where),
            put_premium=float(entry.get("put_premium", 0.0)),
            call_premium=float(entry.get("call_premium", 0.0)),
            client=client,
        )
    except ValueError as error:
        raise BookError(f"{where}: {error}") from error


def _strip(entry, where, volume, market) -> StripStrategy:
    raw_dates = entry.get("settlements")
    if not isinstance(raw_dates, list) or len(raw_dates) == 0:
        raise BookError(f"{where}.settlements must be a list of dates")
    settlements = [_as_date(day, f"{where}.settlements") for day in raw_dates]
    choices = [name for name in ("strike", "strikes", "strike_moneyness") if name in entry]
    if len(choices) != 1:
        raise BookError(f"{where} needs exactly one of strike, strikes, or strike_moneyness")
    if choices[0] == "strike_moneyness":
        if market is None:
            raise BookError(f"{where}.strike_moneyness needs a market")
        return StripStrategy.from_moneyness(
            settlements, market["forward"], float(entry["strike_moneyness"]), volume, _side(entry, where)
        )
    if choices[0] == "strike":
        strikes = [float(entry["strike"])] * len(settlements)
    else:
        raw = entry["strikes"]
        if not isinstance(raw, list):
            raise BookError(f"{where}.strikes must be a list")
        strikes = [float(value) for value in raw]
    try:
        return StripStrategy(tuple(settlements), tuple(strikes), volume, _side(entry, where))
    except ValueError as error:
        raise BookError(f"{where}: {error}") from error


def _seagull(entry, where, volume, market) -> SeagullStrategy:
    maturity = _date(_need(entry, "maturity", where, market), f"{where}.maturity", market)
    settlement = _date(_need(entry, "settlement", where, market), f"{where}.settlement", market)
    forward = _forward(market, settlement, where)
    client = _client(entry, where)
    try:
        return SeagullStrategy(
            maturity=maturity,
            settlement=settlement,
            floor_strike=_wing(entry, where, "floor_strike", "floor_moneyness", forward),
            put_strike=_wing(entry, where, "put_strike", "put_moneyness", forward),
            call_strike=_wing(entry, where, "call_strike", "call_moneyness", forward),
            volume=volume,
            premium_payment_date=_optional_date(entry, "premium_payment_date", where),
            floor_premium=float(entry.get("floor_premium", 0.0)),
            put_premium=float(entry.get("put_premium", 0.0)),
            call_premium=float(entry.get("call_premium", 0.0)),
            client=client,
        )
    except ValueError as error:
        raise BookError(f"{where}: {error}") from error


def _participating(entry, where, volume, market) -> ParticipatingForwardStrategy:
    maturity = _date(_need(entry, "maturity", where, market), f"{where}.maturity", market)
    settlement = _date(_need(entry, "settlement", where, market), f"{where}.settlement", market)
    forward = _forward(market, settlement, where)
    if "participation" not in entry:
        raise BookError(f"{where}.participation is required")
    client = _client(entry, where)
    try:
        return ParticipatingForwardStrategy(
            maturity=maturity,
            settlement=settlement,
            put_strike=_wing(entry, where, "put_strike", "put_moneyness", forward),
            call_strike=_wing(entry, where, "call_strike", "call_moneyness", forward),
            volume=volume,
            participation=float(entry["participation"]),
            premium_payment_date=_optional_date(entry, "premium_payment_date", where),
            put_premium=float(entry.get("put_premium", 0.0)),
            call_premium=float(entry.get("call_premium", 0.0)),
            client=client,
        )
    except ValueError as error:
        raise BookError(f"{where}: {error}") from error


def _client(entry, where) -> Client:
    name = str(entry.get("client", "producer")).lower()
    if name == "producer":
        return Client.PRODUCER
    if name == "consumer":
        return Client.CONSUMER
    raise BookError(f"{where}.client must be producer or consumer")


def _european(entry, where, volume, market):
    maturity = _date(_need(entry, "maturity", where, market), f"{where}.maturity", market)
    settlement = _date(_need(entry, "settlement", where, market), f"{where}.settlement", market)
    strike = _level(entry, where, "strike", "strike_moneyness", market, settlement)
    premium_date = _optional_date(entry, "premium_payment_date", where) or settlement
    return sjd.EuropeanOption(
        maturity, settlement, strike, _put_call(entry, where), volume, premium_date,
        float(entry.get("premium_amount", 0.0)), _side(entry, where),
    )


def _asian(entry, where, volume, market, option):
    dates = _averaging_dates(entry, where, market)
    settlement = _date(_need(entry, "settlement", where, market), f"{where}.settlement", market)
    if "strike" in entry and "strike_moneyness" in entry:
        raise BookError(f"{where} needs exactly one of strike or strike_moneyness")
    if "strike" in entry:
        strike = float(entry["strike"])
    elif "strike_moneyness" in entry:
        if market is None:
            raise BookError(f"{where}.strike_moneyness needs a market")
        average = sum(market["forward"].get_forward(day) for day in dates) / len(dates)
        strike = round(average * float(entry["strike_moneyness"]), 2)
    else:
        raise BookError(f"{where} needs strike or strike_moneyness")
    if option:
        premium_date = _optional_date(entry, "premium_payment_date", where) or settlement
        return sjd.AsianOption(
            dates, settlement, strike, _put_call(entry, where), volume, premium_date,
            float(entry.get("premium_amount", 0.0)), _side(entry, where),
        )
    return sjd.AsianForward(dates, settlement, strike, volume, _side(entry, where))


def _averaging_dates(entry, where, market):
    if "averaging_dates" in entry:
        raw = entry["averaging_dates"]
        if not isinstance(raw, list) or len(raw) == 0:
            raise BookError(f"{where}.averaging_dates must be a list of dates")
        return [_as_date(day, f"{where}.averaging_dates") for day in raw]
    scheduled = _schedule(entry, where, market)
    if "averaging_dates" not in scheduled:
        raise BookError(f"{where}.averaging_dates is required")
    return list(scheduled["averaging_dates"])


def _level(entry, where, absolute, relative, market, settlement):
    if (absolute in entry) == (relative in entry):
        raise BookError(f"{where} needs exactly one of {absolute} or {relative}")
    if absolute in entry:
        strike = float(entry[absolute])
        if strike <= 0.0:
            raise BookError(f"{where}.{absolute} must be positive")
        return strike
    return _wing(entry, where, absolute, relative, _forward(market, settlement, where))


def _wing(entry, where, absolute, relative, forward):
    if (absolute in entry) == (relative in entry):
        raise BookError(f"{where} needs exactly one of {absolute} or {relative}")
    if absolute in entry:
        strike = float(entry[absolute])
    else:
        moneyness = float(entry[relative])
        if moneyness <= 0.0:
            raise BookError(f"{where}.{relative} must be positive")
        if forward is None:
            raise BookError(f"{where}.{relative} needs a market")
        strike = round(forward * moneyness, 2)
    if strike <= 0.0:
        raise BookError(f"{where}.{absolute} must be positive")
    return strike


def _forward(market, settlement, where):
    if market is None:
        return None
    return float(market["forward"].get_forward(settlement))


def _need(entry, field, where, market):
    if field in entry:
        return entry[field]
    scheduled = _schedule(entry, where, market)
    if field not in scheduled:
        raise BookError(f"{where}.{field} is required")
    return scheduled[field]


def _market_view(market):
    if market is None or isinstance(market, dict):
        return market
    from sjd.markets.rates import LoadedMarket

    if isinstance(market, LoadedMarket):
        return {
            "forward": market.forward,
            "discount": market.discount,
            "anchor": market.anchor,
            "environment": market.environment,
        }
    raise BookError("market must be the sample market or a loaded market")


def _schedule(entry, where, market):
    name = entry.get("schedule")
    if name is None:
        return {}
    if market is None:
        raise BookError(f"{where}.schedule needs a market")
    try:
        if name == "collar":
            return {"maturity": market["collar_maturity"], "settlement": market["collar_settlement"]}
        if name == "forward":
            settlement = market["forward_settlement"]
            return {"maturity": settlement, "settlement": settlement}
        if name == "asian":
            return {
                "averaging_dates": list(market["averaging_dates"]),
                "settlement": market["asian_settlement"],
            }
    except KeyError:
        raise BookError(f"{where}.schedule {name!r} is not on this market") from None
    raise BookError(f"{where}.schedule is not collar, forward, or asian")


def _volume(entry, default, where):
    volume = entry.get("volume", default)
    if volume is None:
        raise BookError(f"{where}.volume is required")
    volume = float(volume)
    if volume <= 0.0:
        raise BookError(f"{where}.volume must be positive")
    return volume


def _side(entry, where):
    name = str(entry.get("buy_sell", "")).upper()
    if name == "BUY":
        return sjd.BuySell.BUY
    if name == "SELL":
        return sjd.BuySell.SELL
    raise BookError(f"{where}.buy_sell must be BUY or SELL")


def _put_call(entry, where):
    name = str(entry.get("type", "")).upper()
    if name == "PUT":
        return sjd.PutCall.PUT
    if name == "CALL":
        return sjd.PutCall.CALL
    raise BookError(f"{where}.type must be PUT or CALL")


def _optional_date(entry, field, where):
    if field not in entry or entry[field] is None:
        return None
    return _as_date(entry[field], f"{where}.{field}")


def _date(value, where, market):
    return _as_date(value, where)


def _as_date(value, where):
    if isinstance(value, datetime):
        return value.date()
    if isinstance(value, date):
        return value
    if isinstance(value, str):
        try:
            return date.fromisoformat(value)
        except ValueError as error:
            raise BookError(f"{where} must be a date") from error
    raise BookError(f"{where} must be a date")


def _unknown(table, allowed, where):
    extra = sorted(set(table) - allowed)
    if extra:
        raise BookError(f"{where} has unknown fields: {', '.join(extra)}")
