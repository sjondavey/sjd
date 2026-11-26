"""Parametrised derivative strategies that materialise into contracts.

A strategy is the user-facing book shape (collar, strip, and so on). A CEM
netting set is a ``CemNettingSet``. Simulation ``by_netting_set`` is the value
of each set. ``by_structure`` is the same array.
"""

from __future__ import annotations

import logging
from collections.abc import Sequence
from dataclasses import dataclass
from datetime import date
from enum import Enum

import sjd

logger = logging.getLogger(__name__)


class Client(Enum):
    """Who the bank faces. Every strategy prices the bank's side of the trade.

    A producer sells the commodity. The bank sells the put and buys the call.
    A consumer buys the commodity. The bank buys the put and sells the call.
    """

    PRODUCER = "producer"
    CONSUMER = "consumer"


def _require_client(client: Client) -> Client:
    if not isinstance(client, Client):
        raise ValueError("client must be producer or consumer")
    return client


def _put_call_sides(client: Client) -> tuple[sjd.BuySell, sjd.BuySell]:
    """The bank's put side and call side."""
    _require_client(client)
    if client is Client.PRODUCER:
        return sjd.BuySell.SELL, sjd.BuySell.BUY
    return sjd.BuySell.BUY, sjd.BuySell.SELL


def _signed_option_value(contracts, rates_environment) -> float:
    """Option value on the bank's book. Sold options are negative."""
    total = 0.0
    for contract in contracts:
        sign = 1.0 if contract.buy_sell == sjd.BuySell.BUY else -1.0
        total += sign * contract.get_option_value(rates_environment)
    return total


@dataclass(frozen=True)
class CollarStrategy:
    """A collar on the bank's book.

    Against a producer the bank sells the put and buys the call. Against a
    consumer the bank buys the put and sells the call. Strikes are absolute
    levels. Use ``from_moneyness`` when they should sit relative to the
    forward on the settlement date.
    """

    maturity: date
    settlement: date
    put_strike: float
    call_strike: float
    volume: float
    premium_payment_date: date | None = None
    put_premium: float = 0.0
    call_premium: float = 0.0
    client: Client = Client.PRODUCER

    def __post_init__(self) -> None:
        _require_client(self.client)
        if self.put_strike <= 0.0 or self.call_strike <= 0.0:
            raise ValueError("collar strikes must be positive")
        if self.put_strike >= self.call_strike:
            raise ValueError("put_strike must be below call_strike")
        if self.volume <= 0.0:
            raise ValueError("volume must be positive")

    @classmethod
    def from_moneyness(
        cls,
        maturity: date,
        settlement: date,
        forward: float,
        put_moneyness: float,
        call_moneyness: float,
        volume: float,
        premium_payment_date: date | None = None,
        put_premium: float = 0.0,
        call_premium: float = 0.0,
        client: Client = Client.PRODUCER,
        strike_decimals: int = 2,
    ) -> CollarStrategy:
        """Build a collar with strikes ``forward * moneyness``, rounded."""
        if forward <= 0.0:
            raise ValueError("forward must be positive")
        if not 0.0 < put_moneyness < call_moneyness:
            raise ValueError("need 0 < put_moneyness < call_moneyness")
        return cls(
            maturity=maturity,
            settlement=settlement,
            put_strike=round(forward * put_moneyness, strike_decimals),
            call_strike=round(forward * call_moneyness, strike_decimals),
            volume=volume,
            premium_payment_date=premium_payment_date,
            put_premium=put_premium,
            call_premium=call_premium,
            client=client,
        )

    def contracts(self) -> list:
        """The bank's put and call."""
        premium_date = self.premium_payment_date or self.settlement
        put_side, call_side = _put_call_sides(self.client)
        put = sjd.EuropeanOption(
            maturity=self.maturity,
            settlement=self.settlement,
            strike=self.put_strike,
            type=sjd.PutCall.PUT,
            volume=self.volume,
            premium_payment_date=premium_date,
            premium_amount=self.put_premium,
            buy_sell=put_side,
        )
        call = sjd.EuropeanOption(
            maturity=self.maturity,
            settlement=self.settlement,
            strike=self.call_strike,
            type=sjd.PutCall.CALL,
            volume=self.volume,
            premium_payment_date=premium_date,
            premium_amount=self.call_premium,
            buy_sell=call_side,
        )
        return [put, call]

    def portfolio(self, margin=None):
        """A portfolio of the collar contracts, optionally with a CSA."""
        contracts = self.contracts()
        logger.info(
            "built collar portfolio contracts=%s client=%s", len(contracts), self.client.value
        )
        if margin is None:
            return sjd.Portfolio(contracts)
        return sjd.Portfolio(contracts, margin)

    def portfolio_b2cem(self, factors, credit, optimise=True, margin=None):
        """Collar as a Basel II current-exposure portfolio."""
        contracts = self.contracts()
        logger.info(
            "built collar portfolio contracts=%s client=%s optimise=%s",
            len(contracts),
            self.client.value,
            optimise,
        )
        if margin is None:
            return sjd.PortfolioB2CEM(contracts, factors, credit, optimise)
        return sjd.PortfolioB2CEM(contracts, factors, credit, optimise, margin)

    def premium(self, rates_environment) -> float:
        """The bank's option value. The agreed premium cashflow is not included."""
        return _signed_option_value(self.contracts(), rates_environment)


def _option(maturity, settlement, strike, option_type, volume, premium_date, premium, buy_sell):
    return sjd.EuropeanOption(
        maturity=maturity,
        settlement=settlement,
        strike=strike,
        type=option_type,
        volume=volume,
        premium_payment_date=premium_date or settlement,
        premium_amount=premium,
        buy_sell=buy_sell,
    )


def _b2cem(contracts, factors, credit, optimise, margin):
    if margin is None:
        return sjd.PortfolioB2CEM(contracts, factors, credit, optimise)
    return sjd.PortfolioB2CEM(contracts, factors, credit, optimise, margin)


def _strikes_from_moneyness(dates, forward_curve, moneyness, strike_decimals):
    if moneyness <= 0.0:
        raise ValueError("moneyness must be positive")
    return [round(forward_curve.get_forward(day) * moneyness, strike_decimals) for day in dates]


@dataclass(frozen=True)
class StripStrategy:
    """One physically settled forward on each settlement, with a shared volume.

    ``buy_sell`` is the bank's side. Buying the forward is the bank's trade
    with a producer. ``strikes`` has one entry per settlement. Settlements
    are strictly increasing.
    """

    settlements: tuple[date, ...]
    strikes: tuple[float, ...]
    volume: float
    buy_sell: sjd.BuySell = sjd.BuySell.BUY

    def __post_init__(self) -> None:
        if len(self.settlements) == 0:
            raise ValueError("a strip needs a settlement")
        if len(self.strikes) != len(self.settlements):
            raise ValueError("strikes must match settlements")
        for index in range(len(self.settlements) - 1):
            if self.settlements[index + 1] <= self.settlements[index]:
                raise ValueError("settlements must be strictly increasing")
        if any(strike <= 0.0 for strike in self.strikes):
            raise ValueError("strikes must be positive")
        if self.volume <= 0.0:
            raise ValueError("volume must be positive")

    @classmethod
    def from_moneyness(
        cls,
        settlements,
        forward_curve,
        moneyness: float,
        volume: float,
        buy_sell=sjd.BuySell.BUY,
        strike_decimals: int = 2,
    ) -> StripStrategy:
        """Strike each settlement at ``forward * moneyness``."""
        dates = tuple(settlements)
        strikes = _strikes_from_moneyness(dates, forward_curve, moneyness, strike_decimals)
        return cls(settlements=dates, strikes=tuple(strikes), volume=volume, buy_sell=buy_sell)

    def contracts(self) -> list:
        """One forward per settlement. Maturity equals settlement."""
        return [
            sjd.EuropeanForward(
                settlement=day, strike=strike, volume=self.volume, buy_sell=self.buy_sell
            )
            for day, strike in zip(self.settlements, self.strikes, strict=True)
        ]

    def portfolio(self, margin=None):
        """A portfolio of the strip, optionally with a CSA."""
        contracts = self.contracts()
        logger.info("built strip portfolio contracts=%s", len(contracts))
        if margin is None:
            return sjd.Portfolio(contracts)
        return sjd.Portfolio(contracts, margin)

    def portfolio_b2cem(self, factors, credit, optimise=True, margin=None):
        """Strip as a Basel II current-exposure portfolio."""
        contracts = self.contracts()
        logger.info("built strip portfolio contracts=%s optimise=%s", len(contracts), optimise)
        return _b2cem(contracts, factors, credit, optimise, margin)

    def premium(self, rates_environment) -> float:
        """Forwards have no option premium."""
        return 0.0


@dataclass(frozen=True)
class SeagullStrategy:
    """A seagull on the bank's book: a collar plus a financing put at a lower strike.

    Against a producer the bank sells the put, buys the call, and buys the
    lower put. Against a consumer those sides are reversed. The strikes
    satisfy ``floor_strike < put_strike < call_strike``. All three options
    use the same volume.
    """

    maturity: date
    settlement: date
    floor_strike: float
    put_strike: float
    call_strike: float
    volume: float
    premium_payment_date: date | None = None
    floor_premium: float = 0.0
    put_premium: float = 0.0
    call_premium: float = 0.0
    client: Client = Client.PRODUCER

    def __post_init__(self) -> None:
        _require_client(self.client)
        if min(self.floor_strike, self.put_strike, self.call_strike) <= 0.0:
            raise ValueError("seagull strikes must be positive")
        if not self.floor_strike < self.put_strike < self.call_strike:
            raise ValueError("need floor_strike < put_strike < call_strike")
        if self.volume <= 0.0:
            raise ValueError("volume must be positive")

    @classmethod
    def from_moneyness(
        cls,
        maturity: date,
        settlement: date,
        forward: float,
        floor_moneyness: float,
        put_moneyness: float,
        call_moneyness: float,
        volume: float,
        premium_payment_date: date | None = None,
        floor_premium: float = 0.0,
        put_premium: float = 0.0,
        call_premium: float = 0.0,
        client: Client = Client.PRODUCER,
        strike_decimals: int = 2,
    ) -> SeagullStrategy:
        """Build strikes ``forward * moneyness``, rounded."""
        if forward <= 0.0:
            raise ValueError("forward must be positive")
        if not 0.0 < floor_moneyness < put_moneyness < call_moneyness:
            raise ValueError("need 0 < floor_moneyness < put_moneyness < call_moneyness")
        return cls(
            maturity=maturity,
            settlement=settlement,
            floor_strike=round(forward * floor_moneyness, strike_decimals),
            put_strike=round(forward * put_moneyness, strike_decimals),
            call_strike=round(forward * call_moneyness, strike_decimals),
            volume=volume,
            premium_payment_date=premium_payment_date,
            floor_premium=floor_premium,
            put_premium=put_premium,
            call_premium=call_premium,
            client=client,
        )

    def contracts(self) -> list:
        """The bank's put, call, and lower put."""
        premium_date = self.premium_payment_date
        put_side, call_side = _put_call_sides(self.client)
        floor_side = sjd.BuySell.BUY if put_side == sjd.BuySell.SELL else sjd.BuySell.SELL
        return [
            _option(
                self.maturity, self.settlement, self.put_strike, sjd.PutCall.PUT,
                self.volume, premium_date, self.put_premium, put_side,
            ),
            _option(
                self.maturity, self.settlement, self.call_strike, sjd.PutCall.CALL,
                self.volume, premium_date, self.call_premium, call_side,
            ),
            _option(
                self.maturity, self.settlement, self.floor_strike, sjd.PutCall.PUT,
                self.volume, premium_date, self.floor_premium, floor_side,
            ),
        ]

    def portfolio(self, margin=None):
        """A portfolio of the seagull, optionally with a CSA."""
        contracts = self.contracts()
        logger.info(
            "built seagull portfolio contracts=%s client=%s", len(contracts), self.client.value
        )
        if margin is None:
            return sjd.Portfolio(contracts)
        return sjd.Portfolio(contracts, margin)

    def portfolio_b2cem(self, factors, credit, optimise=True, margin=None):
        """Seagull as a Basel II current-exposure portfolio."""
        contracts = self.contracts()
        logger.info(
            "built seagull portfolio contracts=%s client=%s optimise=%s",
            len(contracts),
            self.client.value,
            optimise,
        )
        return _b2cem(contracts, factors, credit, optimise, margin)

    def premium(self, rates_environment) -> float:
        """The bank's option value. The agreed premium cashflow is not included."""
        return _signed_option_value(self.contracts(), rates_environment)


@dataclass(frozen=True)
class ParticipatingForwardStrategy:
    """A participating forward on the bank's book.

    Against a producer the bank sells a put on the full volume and buys a call
    on the rest. Against a consumer those sides are reversed.
    ``participation`` is the fraction of upside the client keeps, in
    ``(0, 1)``. The call's volume is ``volume * (1 - participation)``. The
    put strike is below the call strike.
    """

    maturity: date
    settlement: date
    put_strike: float
    call_strike: float
    volume: float
    participation: float
    premium_payment_date: date | None = None
    put_premium: float = 0.0
    call_premium: float = 0.0
    client: Client = Client.PRODUCER

    def __post_init__(self) -> None:
        _require_client(self.client)
        if self.put_strike <= 0.0 or self.call_strike <= 0.0:
            raise ValueError("strikes must be positive")
        if self.put_strike >= self.call_strike:
            raise ValueError("put_strike must be below call_strike")
        if self.volume <= 0.0:
            raise ValueError("volume must be positive")
        if not 0.0 < self.participation < 1.0:
            raise ValueError("participation must be between 0 and 1")

    @classmethod
    def from_moneyness(
        cls,
        maturity: date,
        settlement: date,
        forward: float,
        put_moneyness: float,
        call_moneyness: float,
        volume: float,
        participation: float,
        premium_payment_date: date | None = None,
        put_premium: float = 0.0,
        call_premium: float = 0.0,
        client: Client = Client.PRODUCER,
        strike_decimals: int = 2,
    ) -> ParticipatingForwardStrategy:
        """Build strikes ``forward * moneyness``, rounded."""
        if forward <= 0.0:
            raise ValueError("forward must be positive")
        if not 0.0 < put_moneyness < call_moneyness:
            raise ValueError("need 0 < put_moneyness < call_moneyness")
        return cls(
            maturity=maturity,
            settlement=settlement,
            put_strike=round(forward * put_moneyness, strike_decimals),
            call_strike=round(forward * call_moneyness, strike_decimals),
            volume=volume,
            participation=participation,
            premium_payment_date=premium_payment_date,
            put_premium=put_premium,
            call_premium=call_premium,
            client=client,
        )

    def call_volume(self) -> float:
        """Volume of the call."""
        return self.volume * (1.0 - self.participation)

    def contracts(self) -> list:
        """The bank's put on the full volume and call on the participation residual."""
        premium_date = self.premium_payment_date
        put_side, call_side = _put_call_sides(self.client)
        return [
            _option(
                self.maturity, self.settlement, self.put_strike, sjd.PutCall.PUT,
                self.volume, premium_date, self.put_premium, put_side,
            ),
            _option(
                self.maturity, self.settlement, self.call_strike, sjd.PutCall.CALL,
                self.call_volume(), premium_date, self.call_premium, call_side,
            ),
        ]

    def portfolio(self, margin=None):
        """A portfolio of the participating forward, optionally with a CSA."""
        contracts = self.contracts()
        logger.info(
            "built participating forward portfolio contracts=%s client=%s",
            len(contracts),
            self.client.value,
        )
        if margin is None:
            return sjd.Portfolio(contracts)
        return sjd.Portfolio(contracts, margin)

    def portfolio_b2cem(self, factors, credit, optimise=True, margin=None):
        """Participating forward as a Basel II current-exposure portfolio."""
        contracts = self.contracts()
        logger.info(
            "built participating forward portfolio contracts=%s client=%s optimise=%s",
            len(contracts),
            self.client.value,
            optimise,
        )
        return _b2cem(contracts, factors, credit, optimise, margin)

    def premium(self, rates_environment) -> float:
        """The bank's option value. The agreed premium cashflow is not included."""
        return _signed_option_value(self.contracts(), rates_environment)


def sample_collar(
    market,
    forward_curve,
    put_moneyness: float = 0.97,
    call_moneyness: float = 1.03,
    volume: float = 1_000_000.0,
    client: Client = Client.PRODUCER,
) -> CollarStrategy:
    """Collar on the sample market's collar maturity and settlement.

    The default faces a producer, so the bank sells the put and buys the call.
    """
    atm = forward_curve.get_forward(market["collar_settlement"])
    return CollarStrategy.from_moneyness(
        maturity=market["collar_maturity"],
        settlement=market["collar_settlement"],
        forward=atm,
        put_moneyness=put_moneyness,
        call_moneyness=call_moneyness,
        volume=volume,
        client=client,
    )


def __getattr__(name):
    """Schedules and multi-period strips live in ``sjd.structuring``."""
    if name in {
        "Schedule",
        "Period",
        "step_ladder",
        "StructureStrip",
        "ForwardStrip",
        "CollarStrip",
        "SeagullStrip",
        "ParticipatingForwardStrip",
        "solve_pv",
        "compare_structures",
        "plot_structures",
    }:
        from sjd import structuring

        return getattr(structuring, name)
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")


def merge_contracts(*groups: Sequence) -> list:
    """Flatten several contract lists into one book."""
    contracts: list = []
    for group in groups:
        contracts.extend(group)
    return contracts
