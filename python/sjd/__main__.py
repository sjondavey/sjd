"""Run a TOML book on the sample market and write a report folder.

    python -m sjd run book.toml --market sample --paths 10000 --output report
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import sjd
from sjd import examples, reports, stats
from sjd.book import BookError, load_book


class _CapitalView:
    """The pair ``simulate`` reads when a run asks for netting-set values."""

    def __init__(self, portfolio, capital_model):
        self.portfolio = portfolio
        self.capital_model = capital_model


def main(argv=None) -> int:
    """Parse ``argv`` (default ``sys.argv[1:]``) and run the command."""
    parser = argparse.ArgumentParser(prog="python -m sjd")
    commands = parser.add_subparsers(dest="command", required=True)
    run = commands.add_parser("run", help="price a book and write a report folder")
    run.add_argument("book", type=Path)
    run.add_argument("--market", default="sample")
    run.add_argument("--paths", type=int, default=10000)
    run.add_argument("--seed", type=int, default=1)
    run.add_argument("--output", type=Path, default=Path("report"))
    run.add_argument("--default-probability", type=float, default=0.02)
    run.add_argument("--loss-given-default", type=float, default=0.6)
    run.add_argument("--no-capital", action="store_true")
    args = parser.parse_args(argv)
    try:
        _run(args)
    except BookError as error:
        print(error, file=sys.stderr)
        return 2
    return 0


def _run(args) -> None:
    if args.market != "sample":
        raise BookError("only --market sample is supported")
    if args.paths < 1:
        raise BookError("--paths must be at least 1")
    market = examples.sample_market(stochastic=True)
    book = load_book(args.book, market)
    environment = market["environment"]
    portfolio = book.portfolio()
    sjd.require_valid(portfolio, environment)
    factors = sjd.reg_cap_factors("commodity")
    credit = examples.credit_parameters()
    model = sjd.CurrentExposureMethod(portfolio, factors, credit)
    sjd.require_valid(model, environment)
    netting = None if args.no_capital else model.netting_sets()
    # simulate() reads netting sets from an object that holds the portfolio and
    # the capital model. A plain Portfolio has neither.
    simulated = portfolio if netting is None else _CapitalView(portfolio, model)
    simulation = sjd.simulate(
        simulated,
        environment,
        market["simulation_dates"],
        paths=args.paths,
        seed=args.seed,
        by_netting_set=netting is not None,
    )
    dates = stats.exposure_dates(simulation)
    discount = [market["discount"].get_discount_factor(day) for day in dates]
    probability = stats.flat_default_probability(market["anchor"], dates, args.default_probability)
    loss = [args.loss_given_default] * len(dates)
    costs = stats.structure_costs(
        simulation,
        discount,
        probability,
        loss,
        netting_sets=netting,
        factors=None if args.no_capital else factors,
        credit=None if args.no_capital else credit,
    )
    sheets = {
        "prices": reports.prices(book.contracts(), environment),
        "exposure": reports.exposure_profile(simulation),
        "costs": reports.costs_table(costs),
    }
    if netting is not None:
        today = model.capital(portfolio, book.margin, environment)
        sheets["capital"] = reports.capital_table(today)
    reports.write_csv(args.output, sheets)
    try:
        reports.write_excel(args.output / "report.xlsx", sheets)
    except ImportError as error:
        if "openpyxl" not in str(error):
            raise
    print(args.output)


if __name__ == "__main__":
    raise SystemExit(main())
