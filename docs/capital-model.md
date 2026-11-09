# Capital model

Step 14. Regulatory capital is a calculation about a portfolio. It is not a
kind of portfolio.

## What is wrong today

`PortfolioB2CEM` is a `Portfolio` that owns a `CurrentExposureMethod`. The
evolver asks the portfolio for `regulatoryStructures()` when a path should be
valued by netting set. A second capital model would be another portfolio
subclass.

The Basel II add-on is one C++ class per asset class, chosen by a string. The
counterparty weight and the capital ratio are a class with non-const getters.
C++ `Structure` means a CEM netting set, while a user-facing book is a
strategy.

## CapitalModel

`CapitalModel` is not a contract and not a portfolio.

```text
capital(portfolio, margin, environment) -> CapitalResult
```

`CapitalResult` holds the EAD, the RWA, the capital, the replacement cost, the
net-to-gross ratio, and one component per netting set (settlement, volume,
whether it is capped, mark-to-market, and the add-on that remains after that
mark-to-market is taken out).

The C++ method takes `const Portfolio&`, `const Margin*` (null when the book
is not margined), and `const std::shared_ptr<RatesEnvironment>&`. Valuation
already takes the environment that way.

`CurrentExposureMethod` implements the interface. It is built from a portfolio
(or from contracts the caller has already grouped), an `AssetClass`, a
`CounterpartyCredit`, and an optimise flag. It builds its netting sets then.
`capital` values those sets. It does not re-read the portfolio's contract
list, and it reads the margin balance from the pointer passed in, so the
portfolio still owns the account.

Basel II CEM, unchanged:

- Value each netting set once. NGR is net / gross when the net is
  non-negative and the gross is above `1e-14`, otherwise 0.
- Each set contributes `getMtMPlusAddOnForGivenMtM` of that value.
- Unscaled exposure is that sum, minus the net mark-to-market, plus the
  replacement cost: `max(0, net)` unmargined, or `max(0, net - balance)`
  margined.
- EAD is the unscaled exposure floored at 0. RWA is EAD times the
  counterparty risk weight. Capital is RWA times the capital ratio, floored
  at 0. That capital is the number `getRegulatoryCapital` returns today.

SA-CCR is the other implementation, in Step 22. It is not built here. Adding
it does not require a change to `Portfolio`.

## Portfolio and the simulation

`Portfolio` has no capital method and no netting sets. `regulatoryStructures`
is gone.

Per-set mark-to-market is still required for simulated capital. The evolver
takes that list with `setNettingSets`. The sets are the model's contracts
(clones), not the portfolio's. A plain portfolio therefore simulates with no
per-set values. Python `by_netting_set` asks for them. `by_structure` is the
same flag.

The C++ path buffer stays `SimulatedPaths::byStructure`. Python exposes it as
`simulation.by_netting_set`, and `simulation.by_structure` is the same array.

## Supervisory parameters

`AssetClass` is `InterestRate`, `FX`, `Gold`, `Equity`, `PreciousMetal`,
`Commodity`, `Electricity`, `OilGas`, `Metals`, `Agricultural`, and
`OtherCommodity`.

One table holds, per class, the Basel II add-on at the three tenors and the
SA-CCR supervisory factor, correlation, and option volatility (CRE52). CEM
reads only the add-on. The tenors are unchanged: `<= 1y`, `<= 5y`, otherwise.

Gold uses the FX add-on. CEM has no commodity subclass, so `Electricity`,
`OilGas`, `Metals`, `Agricultural`, and `OtherCommodity` use the other-commodity
add-on. Their SA-CCR columns differ. `PreciousMetal` keeps the Annex 4 column
(precious metals excluding gold). SA-CCR has no separate precious-metal row;
that class stores the metals commodity parameters. Interest rate and FX do not
use one correlation in SA-CCR; the stored correlation is 0.

`reg_cap_factors("ir" | "fx" | "equity" | "precious" | "commodity")` still
returns an object whose `get_factor(time)` is the CEM add-on. `gold`,
`electricity`, `oilgas`, `metals`, `agricultural`, and `other` are accepted.
Anything else is a `ConfigurationError`.

## Counterparty

`CounterpartyCredit` is a struct: risk weight and capital ratio. The capital
ratio defaults to 8%. Python `capital_charge` and `counterparty_weighting`
are the same two numbers.

## Names

C++ `Structure` is `CemNettingSet`. The split of a capped payoff from its
wings is unchanged, including a quiet NaN cap on an uncapped set.

`PortfolioB2CEM` is not a C++ class. Python `PortfolioB2CEM` is a deprecated
shim: it builds a `Portfolio` and a `CurrentExposureMethod`, forwards the
book, and returns `capital(...).capital` from `get_regulatory_capital`.
Simulation and the evolver accept the shim and take the netting-set contracts
from the model.
