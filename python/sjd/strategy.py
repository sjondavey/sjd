"""Parametrised derivative strategies that materialise into contracts.

A strategy is the user-facing book shape (collar, strip, and so on). It is
not a C++ ``Structure``: that name still means a regulatory-capital netting
set, and simulation ``by_structure`` still means the same.
"""

from __future__ import annotations

from collections.abc import Sequence
from dataclasses import dataclass
from datetime import date

import sjd


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
        if margin is None:
            return sjd.Portfolio(contracts)
        return sjd.Portfolio(contracts, margin)

    def portfolio_b2cem(self, factors, credit, optimise=True, margin=None):
        """Collar as a Basel II current-exposure portfolio."""
        contracts = self.contracts()
        if margin is None:
            return sjd.PortfolioB2CEM(contracts, factors, credit, optimise)
        return sjd.PortfolioB2CEM(contracts, factors, credit, optimise, margin)

    def premium(self, rates_environment) -> float:
        """Net option premium of the collar (put paid less call received)."""
        put, call = self.contracts()
        return put.get_option_value(rates_environment) + call.get_option_value(rates_environment)


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
