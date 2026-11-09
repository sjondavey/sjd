# Simulation engine

Step 15. Paths can be priced at the same time. A seeded run does not depend on
how many threads do the work.

## What is wrong today

`GbmPathGenerator` (the file was `SingleFactorGMB.h`) owns one `Path` and one
`std::mt19937`. `getRandomPath` draws into that generator, writes the shocks
into the one forward curve, and returns the same path object. The next path
overwrites it. `rollPath` edits the curve objects inside that path rather than
building new ones.

The evolver prices that path before the next draw. Two threads cannot share it.
The margin walk writes the live balance and then puts it back, so two threads
would also race on the account.

`getValue` is already safe to call on one contract from several threads.

## Random numbers

The default sequence stays the current one: `std::mt19937` seeded with the
stored seed, and Box-Muller with two uniforms per normal. Each normal consumes
two `rng()` calls. An antithetic path negates the draws and does not consume
more.

Path `i` is the block of normals starting after `i * steps * 2` uniforms.
A worker that owns paths `[begin, end)` discards that many uniforms once, then
draws forward. One thread and many threads therefore write the same shocks.
The golden paths stay put.

`generatePath(rng, workspace)` draws into a caller-owned workspace. The
generator keeps the seed, for `simulation.seed`, and the prepared template. It
does not keep the engine that a run is drawing from.

`sampling="sobol"` is the other sequence. Dimension is the number of steps.
Sobol points are inverted to normals, then a Brownian bridge on the year
fractions turns them into a path. The shock for a step is the bridge increment
divided by the square root of the step's year fraction, so each shock is still
a unit normal and the early Sobol dimensions set the coarse path. It is
opt-in. It does not replace the default sequence.

## Workspace

A workspace is one path buffer. `workspace()` copies the stochastic forward
curve, drops the prepared path curves, and prepares them again, so each copy
owns the curves `rollPath` edits. Discount and volatility rolls are not edited
by a path and are shared. Fixing rolls are rebuilt per workspace, because a
path writes the fixing rates.

`generatePath` and `generateSpot` write that workspace and return its path.
The caller prices it before the next write.

## Threads

`simulatePaths` and `valueSpotPaths` split the paths across `std::thread`s.
`threads == 0` uses the hardware count, and never more threads than there is
work. Each thread has its own workspace, its own normal buffer, and its own
spot and margin vectors. Contracts and netting sets are shared and only read.

The margin walk uses a local balance. It does not write the portfolio's
account. The balances it returns are the same numbers as before.

## What comes back

The default is unchanged: double arrays for spot, mark-to-market, margin, and,
when asked, per trade and per netting set.

`float32=True` casts those arrays on the way out. The pricing is still double.

`aggregates=True` does not store the per-trade or per-netting-set cubes. It
still prices them when capital is requested, then keeps only the per-step
profiles: mean spot, mean mark-to-market, expected exposure, 95% potential
future exposure of the positive part (the same linear quantile as
`stats.potential_future_exposure`), mean margin, and mean CEM capital when the
run has netting sets. Capital uses the same current-exposure formula as
`stats.regulatory_capital`.

## Names

The file is `GbmPathGenerator`. The class is `GbmPathGenerator`. `factors()`
is 1. A later multi-factor model is another generator, not a different
portfolio. Python `SingleFactorEconomicScenarioGenerator` is the same class.
