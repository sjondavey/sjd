"""Every golden value still matches the pre-modernisation baseline."""

import json

import numpy as np
import pytest
from golden_cases import GOLDEN_PATH, compute

# Closed-form values agree to rounding. Seeded paths use one normal-draw
# implementation, but libm can still differ in the last bits and that compounds
# along a path, so simulation totals use a wider band than the closed form.
RTOL = 1e-9
ATOL = 1e-9
SIM_RTOL = 1e-6
SIM_ATOL = 1e-6


@pytest.fixture(scope="module")
def golden():
    return json.loads(GOLDEN_PATH.read_text(encoding="utf-8"))["values"]


@pytest.fixture(scope="module")
def current():
    return compute()


def test_same_cases(golden, current):
    assert sorted(current) == sorted(golden)


@pytest.mark.parametrize("key", sorted(json.loads(GOLDEN_PATH.read_text(encoding="utf-8"))["values"]))
def test_value(key, golden, current):
    expected = golden[key]
    actual = current[key]
    if key.endswith("dates"):
        assert actual == expected
        return
    rtol, atol = (SIM_RTOL, SIM_ATOL) if key.startswith("simulation.") else (RTOL, ATOL)
    np.testing.assert_allclose(
        np.asarray(actual, dtype=float), np.asarray(expected, dtype=float), rtol=rtol, atol=atol, err_msg=key
    )
