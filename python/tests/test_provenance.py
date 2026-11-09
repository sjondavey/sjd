"""A simulation reports the seed and the build that produced it."""

import re

import numpy as np

import sjd
from sjd import examples, stats


def _book():
    market = examples.sample_market(stochastic=True)
    contracts = examples.sample_contracts(market["forward"], market)["contracts"]
    return market, sjd.Portfolio(contracts)


def test_version_comes_from_the_build():
    info = sjd.build_info()
    assert sjd.__version__ == info["version"]
    assert re.fullmatch(r"\d+\.\d+\.\d+", info["version"])
    assert info["compiler"]
    assert info["build_type"] in {"Debug", "Release", "RelWithDebInfo", "MinSizeRel", "unknown"}
    assert re.fullmatch(r"\d+\.\d+", info["quantlib"])
    assert re.fullmatch(r"\d+\.\d+\.\d+", info["boost"])
    assert info["git_commit"] == "unknown" or re.fullmatch(r"[0-9a-f]{40}", info["git_commit"])
    assert isinstance(info["git_dirty"], bool)


def test_an_omitted_seed_is_reported_and_repeats_the_paths():
    market, portfolio = _book()
    first = sjd.simulate(portfolio, market["environment"], market["simulation_dates"], paths=8)
    assert isinstance(first.seed, int)
    second = sjd.simulate(
        portfolio, market["environment"], market["simulation_dates"], paths=8, seed=first.seed
    )
    assert second.seed == first.seed
    np.testing.assert_array_equal(first.mtm, second.mtm)
    np.testing.assert_array_equal(first.spot, second.spot)


def test_metadata_records_the_run():
    market, portfolio = _book()
    simulation = sjd.simulate(
        portfolio,
        market["environment"],
        market["simulation_dates"],
        paths=6,
        seed=4,
        antithetic=False,
    )
    metadata = simulation.metadata
    assert metadata["version"] == sjd.__version__
    assert metadata["git_commit"] == sjd.build_info()["git_commit"]
    assert metadata["build_type"] == sjd.build_info()["build_type"]
    assert metadata["timings"]["setup_ms"] >= 0
    assert metadata["timings"]["paths_ms"] >= 0
    assert isinstance(metadata["warnings"], list)
    assert all(isinstance(line, str) for line in metadata["warnings"])
    assert metadata["inputs"] == {
        "paths": 6,
        "seed": 4,
        "antithetic": False,
        "by_trade": False,
        "by_structure": False,
        "holding_period": 0,
    }


def test_summary_and_structure_costs_can_carry_the_metadata():
    market, portfolio = _book()
    simulation = sjd.simulate(
        portfolio, market["environment"], market["simulation_dates"], paths=6, seed=4, antithetic=False
    )
    plain = stats.summary(simulation)
    assert "metadata" not in plain
    with_metadata = stats.summary(simulation, metadata=True)
    assert with_metadata["metadata"]["inputs"]["seed"] == 4

    dates = stats.exposure_dates(simulation)
    ones = [1.0] * len(dates)
    zeros = [0.0] * len(dates)
    costs = stats.structure_costs(simulation, ones, zeros, ones)
    assert "metadata" not in costs
    traced = stats.structure_costs(simulation, ones, zeros, ones, metadata=True)
    assert traced["metadata"]["git_commit"] == sjd.build_info()["git_commit"]
