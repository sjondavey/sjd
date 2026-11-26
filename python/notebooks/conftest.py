"""Run the rates notebook before the structuring notebook that reads its database."""

from __future__ import annotations


def pytest_collection_modifyitems(items):
    def order(item):
        name = item.path.name
        if name == "xau_usd_rates.ipynb":
            return "xau_usd_3y.ipynb", 0
        if name == "xau_usd_3y.ipynb":
            return "xau_usd_3y.ipynb", 1
        return name, 0

    items.sort(key=order)
