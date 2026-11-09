# Immutable contracts

Step 13. A contract is built with its terms and then only read. `getValue` does
not store anything on the contract.

## What is wrong today

`SingleOptionContract::getValue` keeps the last premium, the `RatesEnvironment`
pointer, and that environment's market revision. The method cannot be `const`,
and two threads cannot price the same contract. The cache also misses an edit
inside a curve that is already attached: only replacing the curve pointer
advances the revision.

The stored Black '76 and average-rate formula objects are the same kind of
state. Every valuation calls `setParameters` on them.

Public setters (`setVolume`, `setStrike`, `setType`, and the rest) change those
terms after construction and clear the cache. The structure splitter uses them
on clones while it is still building a netting set.

## Contract

- Terms are constructor arguments. There is no public setter.
- `getValue`, `getOptionValue`, and `getPayoff` are `const`. They take
  `const std::shared_ptr<RatesEnvironment>&` and always recompute.
- A European or Asian option builds its formula on the stack for that call,
  using the unchecked formula inputs the Monte Carlo path already uses. The
  formula is not a member.
- `clone()` is `const`.
- `withVolume` and `withPremiumAmount` return a new contract. The structure
  splitter uses them when it scales a wing or lifts a premium into a cashflow.
  The original contract is unchanged. The splitter replaces its own pointer
  with the new one, including the pointer still held in the capped book.
- The validation issue tracker is the one mutable member. `validate` writes it.
  `getValue` does not. Concurrent `getValue` on one contract is safe.
  Concurrent `validate` is not.

`RatesEnvironment::marketRevision` stays. Contracts no longer read it. A later
caller-side cache can.

## Where a value is reused

Removing the cache must not price a trade twice on a path that asks for
per-trade values.

- The evolver prices each underlying trade once per step. The sum is the
  portfolio MtM. When `by_trade` is set, those same numbers are stored.
  Netting-set values are a separate pass: a structure holds clones, not the
  portfolio's contracts, and each structure is priced once.
- `CurrentExposureMethod::getRegulatoryCapital` prices each netting set once
  and passes that mark-to-market into `getMtMPlusAddOnForGivenMtM`. It does
  not price the set again to recover the number it just computed.

## Out of scope

Curves, the margin balance, the scenario generator, and the formula classes
keep their setters. Those objects are not contracts. Regulatory capital stays
on `PortfolioB2CEM` until Step 14.
