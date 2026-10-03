-- Pillars for the markets in python/markets/catalogue.toml.
--
-- Conventions stay in the catalogue: interpolation, extrapolation, calendars,
-- the GOFO conversion, and the ZAR metal cross. This database stores the
-- numbers for one anchor date (the as-of date of the curves).
--
-- The key tables (currency, market_quote, stored_forward, stored_spot) are
-- filled from the catalogue by schema.initialize. The pillar tables point at
-- them, so a pillar for a quote the catalogue does not store is rejected.
-- A ZAR metal forward, for example, is not a row in stored_forward: it is the
-- USD metal forward times USDZAR.

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS schema_info (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    version INTEGER NOT NULL
);

INSERT INTO schema_info (id, version) VALUES (1, 2)
ON CONFLICT (id) DO NOTHING;

-- Settlement currencies. Each has one discount curve.
CREATE TABLE IF NOT EXISTS currency (
    code TEXT PRIMARY KEY
);

-- Every market and settlement currency in the catalogue. Each has a surface.
CREATE TABLE IF NOT EXISTS market_quote (
    market TEXT NOT NULL,
    currency TEXT NOT NULL REFERENCES currency (code),
    PRIMARY KEY (market, currency)
);

-- Quotes whose forward pillars are stored rather than derived.
CREATE TABLE IF NOT EXISTS stored_forward (
    market TEXT NOT NULL,
    currency TEXT NOT NULL,
    PRIMARY KEY (market, currency),
    FOREIGN KEY (market, currency) REFERENCES market_quote (market, currency)
);

-- Quotes that need a spot, such as GOFO quotes, which store lease rates.
CREATE TABLE IF NOT EXISTS stored_spot (
    market TEXT NOT NULL,
    currency TEXT NOT NULL,
    PRIMARY KEY (market, currency),
    FOREIGN KEY (market, currency) REFERENCES market_quote (market, currency)
);

-- GOFO lease rate, or an outright forward, depending on the quote spec.
-- A calendar-month pillar_date is identified by its month and year.
CREATE TABLE IF NOT EXISTS forward_pillar (
    anchor_date TEXT NOT NULL CHECK (anchor_date GLOB '????-??-??'),
    market TEXT NOT NULL,
    currency TEXT NOT NULL,
    pillar_date TEXT NOT NULL CHECK (pillar_date GLOB '????-??-??'),
    value REAL NOT NULL,
    PRIMARY KEY (anchor_date, market, currency, pillar_date),
    FOREIGN KEY (market, currency) REFERENCES stored_forward (market, currency)
);

-- Put delta in percent (50 is at the money), or moneyness, per the vol spec.
CREATE TABLE IF NOT EXISTS volatility_pillar (
    anchor_date TEXT NOT NULL CHECK (anchor_date GLOB '????-??-??'),
    market TEXT NOT NULL,
    currency TEXT NOT NULL,
    pillar_date TEXT NOT NULL CHECK (pillar_date GLOB '????-??-??'),
    strike_dimension REAL NOT NULL,
    volatility REAL NOT NULL CHECK (volatility > 0),
    PRIMARY KEY (anchor_date, market, currency, pillar_date, strike_dimension),
    FOREIGN KEY (market, currency) REFERENCES market_quote (market, currency)
);

CREATE TABLE IF NOT EXISTS discount_pillar (
    anchor_date TEXT NOT NULL CHECK (anchor_date GLOB '????-??-??'),
    currency TEXT NOT NULL REFERENCES currency (code),
    pillar_date TEXT NOT NULL CHECK (pillar_date GLOB '????-??-??'),
    discount_factor REAL NOT NULL CHECK (discount_factor > 0),
    PRIMARY KEY (anchor_date, currency, pillar_date)
);

CREATE TABLE IF NOT EXISTS spot (
    anchor_date TEXT NOT NULL CHECK (anchor_date GLOB '????-??-??'),
    market TEXT NOT NULL,
    currency TEXT NOT NULL,
    value REAL NOT NULL CHECK (value > 0),
    PRIMARY KEY (anchor_date, market, currency),
    FOREIGN KEY (market, currency) REFERENCES stored_spot (market, currency)
);
