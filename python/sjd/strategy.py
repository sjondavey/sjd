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

import sjd

logger = logging.getLogger(__name__)


@dataclass(frozen=True)
class CollarStrategy:
    """Buy a put and sell a call on the same maturity, settlement, and volume.

    Strikes are absolute levels. Use ``from_moneyness`` when they should sit
    relative to the forward on the settlement date.
    """

    maturity: date
    settlement: date
    put_strike: float
    call_strike: float
    volume: float
    premium_payment_date: date | None = None
    put_premium: float = 0.0
    call_premium: float = 0.0

    def __post_init__(self) -> None:
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
        )

    def contracts(self) -> list:
        """European put (buy) and call (sell) that make up the collar."""
        premium_date = self.premium_payment_date or self.settlement
        put = sjd.EuropeanOption(
            maturity=self.maturity,
            settlement=self.settlement,
            strike=self.put_strike,
            type=sjd.PutCall.PUT,
            volume=self.volume,
            premium_payment_date=premium_date,
            premium_amount=self.put_premium,
            buy_sell=sjd.BuySell.BUY,
        )
        call = sjd.EuropeanOption(
            maturity=self.maturity,
            settlement=self.settlement,
            strike=self.call_strike,
            type=sjd.PutCall.CALL,
            volume=self.volume,
            premium_payment_date=premium_date,
            premium_amount=self.call_premium,
            buy_sell=sjd.BuySell.SELL,
        )
        return [put, call]

    def portfolio(self, margin=None):
        """A portfolio of the collar contracts, optionally with a CSA."""
        contracts = self.contracts()
        logger.info("built collar portfolio contracts=%s", len(contracts))
        if margin is None:
            return sjd.Portfolio(contracts)
        return sjd.Portfolio(contracts, margin)

    def portfolio_b2cem(self, factors, credit, optimise=True, margin=None):
        """Collar as a Basel II current-exposure portfolio."""
        contracts = self.contracts()
        logger.info("built collar portfolio contracts=%s optimise=%s", len(contracts), optimise)
        if margin is None:
            return sjd.PortfolioB2CEM(contracts, factors, credit, optimise)
        return sjd.PortfolioB2CEM(contracts, factors, credit, optimise, margin)

    def premium(self, rates_environment) -> float:
        """Net option premium of the collar (put paid less call received)."""
        put, call = self.contracts()
        return put.get_option_value(rates_environment) + call.get_option_value(rates_environment)


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

    ``strikes`` has one entry per settlement. Settlements are strictly
    increasing.
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
    """Buy a put, sell a call, and sell a lower put, on one maturity.

    The strikes satisfy ``floor_strike < put_strike < call_strike``. The sold
    lower put is the financing wing. All three options use the same volume.
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

    def __post_init__(self) -> None:
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
        )

    def contracts(self) -> list:
        """Bought put, sold call, and sold floor put."""
        premium_date = self.premium_payment_date
        return [
            _option(
                self.maturity, self.settlement, self.put_strike, sjd.PutCall.PUT,
                self.volume, premium_date, self.put_premium, sjd.BuySell.BUY,
            ),
            _option(
                self.maturity, self.settlement, self.call_strike, sjd.PutCall.CALL,
                self.volume, premium_date, self.call_premium, sjd.BuySell.SELL,
            ),
            _option(
                self.maturity, self.settlement, self.floor_strike, sjd.PutCall.PUT,
                self.volume, premium_date, self.floor_premium, sjd.BuySell.SELL,
            ),
        ]

    def portfolio(self, margin=None):
        """A portfolio of the seagull, optionally with a CSA."""
        contracts = self.contracts()
        logger.info("built seagull portfolio contracts=%s", len(contracts))
        if margin is None:
            return sjd.Portfolio(contracts)
        return sjd.Portfolio(contracts, margin)

    def portfolio_b2cem(self, factors, credit, optimise=True, margin=None):
        """Seagull as a Basel II current-exposure portfolio."""
        contracts = self.contracts()
        logger.info("built seagull portfolio contracts=%s optimise=%s", len(contracts), optimise)
        return _b2cem(contracts, factors, credit, optimise, margin)

    def premium(self, rates_environment) -> float:
        """Net option premium (bought put, less the two sold options)."""
        return sum(option.get_option_value(rates_environment) for option in self.contracts())


@dataclass(frozen=True)
class ParticipatingForwardStrategy:
    """Buy a put on the full volume and sell a call on the rest.

    ``participation`` is the fraction of upside kept, in ``(0, 1)``. The sold
    call's volume is ``volume * (1 - participation)``. The put strike is below
    the call strike.
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

    def __post_init__(self) -> None:
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
        )

    def call_volume(self) -> float:
        """Volume of the sold call."""
        return self.volume * (1.0 - self.participation)

    def contracts(self) -> list:
        """Bought put on the full volume, sold call on the participation residual."""
        premium_date = self.premium_payment_date
        return [
            _option(
                self.maturity, self.settlement, self.put_strike, sjd.PutCall.PUT,
                self.volume, premium_date, self.put_premium, sjd.BuySell.BUY,
            ),
            _option(
                self.maturity, self.settlement, self.call_strike, sjd.PutCall.CALL,
                self.call_volume(), premium_date, self.call_premium, sjd.BuySell.SELL,
            ),
        ]

    def portfolio(self, margin=None):
        """A portfolio of the participating forward, optionally with a CSA."""
        contracts = self.contracts()
        logger.info("built participating forward portfolio contracts=%s", len(contracts))
        if margin is None:
            return sjd.Portfolio(contracts)
        return sjd.Portfolio(contracts, margin)

    def portfolio_b2cem(self, factors, credit, optimise=True, margin=None):
        """Participating forward as a Basel II current-exposure portfolio."""
        contracts = self.contracts()
        logger.info(
            "built participating forward portfolio contracts=%s optimise=%s", len(contracts), optimise
        )
        return _b2cem(contracts, factors, credit, optimise, margin)

    def premium(self, rates_environment) -> float:
        """Net option premium (put paid less call received)."""
        return sum(option.get_option_value(rates_environment) for option in self.contracts())


def sample_collar(
    market,
    forward_curve,
    put_moneyness: float = 0.97,
    call_moneyness: float = 1.03,
    volume: float = 1_000_000.0,
) -> CollarStrategy:
    """Collar on the sample market's collar maturity and settlement."""
    atm = forward_curve.get_forward(market["collar_settlement"])
    return CollarStrategy.from_moneyness(
        maturity=market["collar_maturity"],
        settlement=market["collar_settlement"],
        forward=atm,
        put_moneyness=put_moneyness,
        call_moneyness=call_moneyness,
        volume=volume,
    )


def merge_contracts(*groups: Sequence) -> list:
    """Flatten several contract lists into one book."""
    contracts: list = []
    for group in groups:
        contracts.extend(group)
    return contracts
