"""C++ records reach Python's logging module."""

import logging
from datetime import date

import pytest

import sjd
from sjd import examples


@pytest.fixture
def info_logging():
    sjd.set_log_level("INFO")
    yield
    sjd.set_log_level("INFO")
    logging.getLogger("sjd").setLevel(logging.NOTSET)


def test_simulation_writes_start_and_end(caplog, info_logging):
    market = examples.sample_market(stochastic=True)
    contracts = examples.sample_contracts(market["forward"], market)["contracts"]
    portfolio = sjd.Portfolio(contracts)
    with caplog.at_level(logging.INFO):
        sjd.simulate(portfolio, market["environment"], market["simulation_dates"], paths=4, seed=1)
    assert "simulation start" in caplog.text
    assert "simulation end" in caplog.text


def test_failed_delta_solve_writes_a_warning(caplog, info_logging):
    anchor = date(2012, 1, 27)
    observations = [
        date(2012, 2, 27),
        date(2012, 3, 27),
        date(2012, 4, 27),
        date(2012, 7, 27),
        date(2013, 1, 27),
        date(2014, 1, 27),
    ]
    row = [0.18, 0.18, 0.18, 0.20, 0.23, 0.25]
    surface = sjd.VolatilitySurfaceDelta(
        anchor,
        observations,
        [10, 25, 50, 75, 90],
        [row, row, row, row, row],
        sjd.SurfaceInterpolator.BICUBIC,
    )
    with caplog.at_level(logging.WARNING), pytest.raises(sjd.NumericalError):
        surface.strike_from_delta(date(2013, 1, 27), -1.0, 100.0)
    assert "Could not convert a delta to a strike" in caplog.text


def test_cem_optimisation_writes_an_info_record(caplog, info_logging):
    market = examples.sample_market(stochastic=True)
    contracts = examples.sample_contracts(market["forward"], market)["contracts"]
    with caplog.at_level(logging.INFO):
        sjd.PortfolioB2CEM(contracts, sjd.reg_cap_factors("commodity"), examples.credit_parameters(), True)
    assert "CEM optimisation" in caplog.text
