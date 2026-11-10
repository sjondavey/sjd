# CEM capital

Regulatory capital today is the Basel II current-exposure method.
`CurrentExposureMethod` implements it for a portfolio on one asset class.
`sjd.stats.regulatory_capital` applies the same add-on along a simulation.
SA-CCR is not implemented. The supervisory table already stores its
factor, correlation, and option volatility. CEM does not read them.

## Netting sets

Contracts are grouped by settlement date. Two derivatives in one group
must share a maturity. Two Asians must also share their averaging dates.
A cashflow only has to share the settlement.

`CemNettingSet` then looks at the payoff, which is piecewise linear
between strikes.

- If the payoff is capped, or the largest absolute slope sits in an
  uncapped wing, the group is one netting set.
- Otherwise the payoff is split into a capped part and an uncapped part.
  That is two netting sets.

The volume used for the add-on is that largest absolute slope, not the
sum of the contract notionals. A collar bought and sold at the same volume
therefore contributes one volume, not two.

A set with no price exposure contributes its cap and no add-on. An
uncapped set is not limited by a cap. Its stored cap is not a number.
Use the capped flag.

A group that does not qualify, or a payoff that is not piecewise linear,
is a validation error. The capital is not silently dropped.

## Net-to-gross ratio

Each set is valued once. The net is the sum of those values. The gross is
the sum of the positive values.

The ratio is net / gross when the net is non-negative and the gross is
above `1e-14`. Otherwise the ratio is 0. A negative net does not produce
a negative ratio.

## Add-on

For a set with price exposure, still on or before its settlement:

```text
simple = forward(value date) * volume * factor(years)
add-on = simple * (0.4 + 0.6 * ratio)
```

`years` is the calendar-day gap from the value date to the settlement,
divided by 365. The factor is the CEM column for the model's asset class:

| Years to settlement | Factor |
| --- | --- |
| `<= 1` | one-year column |
| `<= 5` | five-year column |
| otherwise | beyond five years |

On the anchor date the forward is the spot. Along a path the value date
is that path date, and the forward at the anchor of the rolled
environment is the path spot. `regulatory_capital` uses `simulation.spot`
for the same quantity.

The set's contribution is its mark-to-market plus the add-on. If the set
is capped, that sum is limited to the cap. After settlement the
contribution is 0.

CEM factors stored in the table:

| Asset class | `<= 1y` | `<= 5y` | longer |
| --- | --- | --- | --- |
| Interest rate | 0 | 0.5% | 1.5% |
| FX, and gold | 1% | 5% | 7.5% |
| Equity | 6% | 8% | 10% |
| Precious metal | 7% | 7% | 8% |
| Commodity, electricity, oil/gas, metals, agricultural, other | 10% | 12% | 15% |

CEM has no commodity subclass, so electricity and the other commodity
rows share the commodity add-on. Their SA-CCR columns differ, and CEM
does not use those columns.

## Margin, EAD, capital

Replacement cost is `max(0, net)` when the book is not margined, and
`max(0, net - margin balance)` when it is. Today's `capital` reads the
balance it is given. It does not generate a new margin call. Along a
path the balance is the posted margin on that date.

```text
unscaled = sum(contribution) - net + replacement
EAD       = max(0, unscaled)
RWA       = EAD * risk weight
capital   = max(0, unscaled * capital ratio * risk weight)
```

The risk weight and the capital ratio come from `CounterpartyCredit`.
The Python helpers default to a weight of 1 and a ratio of 0.08.
`structure_costs` takes the mean of this capital across paths when
netting sets, factors, and credit are all supplied. The charge is not
delta-aware: a collar and an outright with the same effective volume
draw the same add-on.
