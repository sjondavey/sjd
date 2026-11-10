"""Report tables for a priced book.

The frames are pandas DataFrames. Excel export needs openpyxl. Neither is a
dependency of the core package: install the ``front`` extra.
"""

from __future__ import annotations

from pathlib import Path


def prices(contracts, rates_environment):
    """One row per contract: kind, settlement, volume, buy/sell, and value."""
    pandas = _pandas()
    rows = []
    for contract in contracts:
        rows.append(
            {
                "kind": type(contract).__name__,
                "settlement": contract.settlement_date(),
                "volume": contract.volume,
                "buy_sell": contract.buy_sell.name,
                "value": contract.get_value(rates_environment),
            }
        )
    return pandas.DataFrame(rows)


def exposure_profile(simulation):
    """One row per requested simulation date."""
    pandas = _pandas()
    from sjd import stats

    dates = stats.exposure_dates(simulation)
    values = stats.exposure(simulation)[:, simulation.exposure_steps]
    return pandas.DataFrame(
        {
            "date": dates,
            "expected_exposure": stats.expected_exposure(values),
            "negative_expected_exposure": stats.negative_expected_exposure(values),
            "mean_mtm": values.mean(axis=0),
        }
    )


def costs_table(costs):
    """One row of a ``structure_costs`` result."""
    pandas = _pandas()
    keys = ("cva", "dva", "funding", "capital", "capital_cost", "total")
    return pandas.DataFrame([{key: costs[key] for key in keys}])


def capital_table(result):
    """One row per netting set, with the book totals repeated on each row."""
    pandas = _pandas()
    rows = []
    for component in result.components:
        rows.append(
            {
                "settlement": component["settlement"],
                "volume": component["volume"],
                "capped": component["capped"],
                "mark_to_market": component["mark_to_market"],
                "add_on": component["add_on"],
                "ead": result.ead,
                "rwa": result.rwa,
                "capital": result.capital,
            }
        )
    return pandas.DataFrame(rows)


def write_csv(folder, sheets) -> None:
    """Write each frame to ``<name>.csv`` in ``folder``."""
    destination = Path(folder)
    destination.mkdir(parents=True, exist_ok=True)
    for name, frame in sheets.items():
        frame.to_csv(destination / f"{name}.csv", index=False)


def write_excel(path, sheets) -> None:
    """Write each frame to a sheet. Requires openpyxl."""
    _pandas()
    try:
        import openpyxl  # noqa: F401
    except ImportError as error:
        raise ImportError("openpyxl is required to write Excel; install the sjd front extra") from error
    from pandas import ExcelWriter

    with ExcelWriter(path, engine="openpyxl") as writer:
        for name, frame in sheets.items():
            frame.to_excel(writer, sheet_name=name, index=False)


def _pandas():
    try:
        import pandas
    except ImportError as error:
        raise ImportError("pandas is required for reports; install the sjd front extra") from error
    return pandas
