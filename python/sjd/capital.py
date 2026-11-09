"""A book and the capital model that prices it.

``PortfolioB2CEM`` is the deprecated constructor. New code builds a
``Portfolio`` and a ``CurrentExposureMethod``.
"""

import warnings

import sjd._core as _core


class PortfolioB2CEM:
    """Deprecated. A ``Portfolio`` plus a ``CurrentExposureMethod``.

    Methods of the book are forwarded to the portfolio. ``netting_sets`` and
    ``get_regulatory_capital`` come from the capital model.
    """

    def __init__(self, contracts, factors, credit, optimise=False, margin=None):
        warnings.warn(
            "PortfolioB2CEM is deprecated; build a Portfolio and a CurrentExposureMethod",
            DeprecationWarning,
            stacklevel=2,
        )
        self._margin = margin
        if margin is None:
            self.portfolio = _core.Portfolio(contracts)
        else:
            self.portfolio = _core.Portfolio(contracts, margin)
        self.capital_model = _core.CurrentExposureMethod(self.portfolio, factors, credit, optimise)

    def __getattr__(self, name):
        return getattr(self.portfolio, name)

    def netting_sets(self):
        return self.capital_model.netting_sets()

    def netting_set_contracts(self):
        return self.capital_model.netting_set_contracts()

    def get_regulatory_capital(self, rates_environment):
        result = self.capital_model.capital(self.portfolio, self._margin, rates_environment)
        return result.capital

    def validate(self, rates_environment=None):
        if rates_environment is None:
            book = self.portfolio.validate()
            model = self.capital_model.validate()
        else:
            book = self.portfolio.validate(rates_environment)
            model = self.capital_model.validate(rates_environment)
        if not book.ok:
            return book
        return model

    def validate_report(self, rates_environment=None):
        return self.validate(rates_environment)
