-- Pillars for the markets in python/sjd/markets/catalogue.toml.
--
-- Conventions stay in the catalogue: interpolation, extrapolation, calendars,
-- the GOFO conversion, and the ZAR metal cross. Numbers belong to a rates
-- label. The label is a name plus the datetime it was saved, and it has one
-- anchor date. Every discount curve, forward, spot, and volatility under that
-- label uses that anchor. A second commodity is another quote on the same
-- label. A later save of the same name is a new datetime, and loading the
-- name without a datetime returns the newest one.
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

INSERT INTO schema_info (id, version) VALUES (1, 3)
ON CONFLICT (id) DO NOTHING;

-- One snapshot of a rates environment. as_of is YYYY-MM-DDTHH:MM:SS, so the
-- newest text is the newest instance. anchor_date is the curve date.
CREATE TABLE IF NOT EXISTS rates_label (
    name TEXT NOT NULL CHECK (length(name) > 0),
    as_of TEXT NOT NULL CHECK (as_of GLOB '????-??-??T??:??:??'),
    anchor_date TEXT NOT NULL CHECK (anchor_date GLOB '????-??-??'),
    PRIMARY KEY (name, as_of)
);

CREATE TRIGGER IF NOT EXISTS rates_label_anchor_fixed
BEFORE UPDATE OF anchor_date ON rates_label
FOR EACH ROW
WHEN NEW.anchor_date != OLD.anchor_date
BEGIN
    SELECT RAISE(ABORT, 'a rates label anchor date cannot change');
END;

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
    label_name TEXT NOT NULL,
    as_of TEXT NOT NULL,
    market TEXT NOT NULL,
    currency TEXT NOT NULL,
    pillar_date TEXT NOT NULL CHECK (pillar_date GLOB '????-??-??'),
    value REAL NOT NULL,
    PRIMARY KEY (label_name, as_of, market, currency, pillar_date),
    FOREIGN KEY (label_name, as_of) REFERENCES rates_label (name, as_of),
    FOREIGN KEY (market, currency) REFERENCES stored_forward (market, currency)
);

-- Put delta in percent (50 is at the money), or moneyness, per the vol spec.
CREATE TABLE IF NOT EXISTS volatility_pillar (
    label_name TEXT NOT NULL,
    as_of TEXT NOT NULL,
    market TEXT NOT NULL,
    currency TEXT NOT NULL,
    pillar_date TEXT NOT NULL CHECK (pillar_date GLOB '????-??-??'),
    strike_dimension REAL NOT NULL,
    volatility REAL NOT NULL CHECK (volatility > 0),
    PRIMARY KEY (label_name, as_of, market, currency, pillar_date, strike_dimension),
    FOREIGN KEY (label_name, as_of) REFERENCES rates_label (name, as_of),
    FOREIGN KEY (market, currency) REFERENCES market_quote (market, currency)
);

CREATE TABLE IF NOT EXISTS discount_pillar (
    label_name TEXT NOT NULL,
    as_of TEXT NOT NULL,
    currency TEXT NOT NULL REFERENCES currency (code),
    pillar_date TEXT NOT NULL CHECK (pillar_date GLOB '????-??-??'),
    discount_factor REAL NOT NULL CHECK (discount_factor > 0),
    PRIMARY KEY (label_name, as_of, currency, pillar_date),
    FOREIGN KEY (label_name, as_of) REFERENCES rates_label (name, as_of)
);

CREATE TABLE IF NOT EXISTS spot (
    label_name TEXT NOT NULL,
    as_of TEXT NOT NULL,
    market TEXT NOT NULL,
    currency TEXT NOT NULL,
    value REAL NOT NULL CHECK (value > 0),
    PRIMARY KEY (label_name, as_of, market, currency),
    FOREIGN KEY (label_name, as_of) REFERENCES rates_label (name, as_of),
    FOREIGN KEY (market, currency) REFERENCES stored_spot (market, currency)
);
