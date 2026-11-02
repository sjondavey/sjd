"""Every golden value still matches the pre-modernisation baseline."""

import json

import numpy as np
import pytest

from golden_cases import GOLDEN_PATH, compute

# Closed-form values and seeded paths agree to rounding on one platform. Across
# compilers the maths library can differ in the last bits, which compounds
# along a path, so the tolerance is relative 1e-9 rather than exact.
RTOL = 1e-9
ATOL = 1e-9


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
    np.testing.assert_allclose(
        np.asarray(actual, dtype=float), np.asarray(expected, dtype=float), rtol=RTOL, atol=ATOL, err_msg=key
    )
