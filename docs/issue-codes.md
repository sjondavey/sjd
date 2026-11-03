# Validation issue codes

These identifiers are the `code` field of a `ValidationIssue`. The constants live in
`include/sjd/tools/IssueCodes.h`. `ERROR` fails `isOK()`. `WARNING` does not.

| Code | Meaning |
| --- | --- |
| `ASIAN_AVERAGING_DATES_MISSING` | Averaging dates not set |
| `ASIAN_FIXING_DATE_MISSING` | A fixing date required by the averaging schedule is missing |
| `ASIAN_MATURITY_NOT_FINAL_AVERAGE` | Maturity date does not equal the final averaging date |
| `ASIAN_SETTLEMENT_BEFORE_FINAL_AVERAGE` | Settlement date before the final averaging date |
| `CASHFLOW_MATURITY_NOT_SETTLEMENT` | Maturity Date must equal Settlement Date |
| `CONTRACT_EXTRACT_FAILED` | Cannot extract single instruments from input contracts |
| `CONTRACT_MATURITY_AFTER_SETTLEMENT` | Maturity date after settlement date |
| `CONTRACT_PREMIUM_DATE_AFTER_SETTLEMENT` | Premium payment date after settlement date. Consider using a cash flow and an  |
| `CONTRACT_PREMIUM_NEGATIVE` | Premium Amount must be stored as a non-negative number |
| `CONTRACT_STRIKE_NEGATIVE` | Strike is negative |
| `CURVE_INPUT_EMPTY` | Curve observation times are empty |
| `CURVE_OBSERVATION_BEFORE_ANCHOR` | First observation is before the anchor date |
| `CURVE_OBSERVATION_BEFORE_ZERO` | First observation time is before zero |
| `CVA_DATES_BEFORE_ANCHOR` | Observation dates before anchor date |
| `CVA_LGD_INTERPOLATOR` | Loss-given-default interpolator is not usable |
| `CVA_LGD_RANGE` | LDG is not in the range [0,1] / Loss given default must be between 0 and 1 |
| `CVA_PD_ANCHOR_NOT_ZERO` | Cumulative default probability on the anchor date is not 0 |
| `CVA_PD_INTERPOLATOR` | Default-probability interpolator is not usable |
| `CVA_PD_NOT_INCREASING` | Cumulative default probabilities are not strictly increasing |
| `CVA_PD_RANGE` | PD is not in the range [0,1] / Cumulative default probability not in the range [0, 1] |
| `DISCOUNT_CURVE_DIMENSION_MISMATCH` | discount curve dimension mismatch |
| `DISCOUNT_CURVE_INVALID` | Discount curve inputs are not usable |
| `DISCOUNT_CURVE_NEGATIVE_FACTOR` | has negative inputs |
| `DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH` | Discount rate source does not have the correct anchor date |
| `DISCOUNT_RATE_SOURCE_RANGE` | First forward date is outside the rate source range / Last forward date is outside the rate source range |
| `FIXING_DATES_AND_RATES_LENGTH` | Number of Dates and Rates inputs are not equal |
| `FIXING_DATES_EMPTY` | No fixing dates entered |
| `FIXING_DATES_NOT_INCREASING` | Input Dates are not strictly increasing |
| `FIXING_FIRST_DATE_MISMATCH` | First Fixing Date does not match the first date in the fixing vector |
| `FIXING_RATE_OUT_OF_RANGE` | A fixing rate is outside [0, 1] |
| `FOREIGN_DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH` | Foreign discount rate source does not have the correct anchor date |
| `FORWARD_CURVE_CMA_DIMENSION_MISMATCH` | forward curve cma dimension mismatch |
| `FORWARD_CURVE_CMA_INVALID` | Calendar-month-average forward curve inputs are not usable |
| `FORWARD_CURVE_DIMENSION_MISMATCH` | forward curve dimension mismatch |
| `FORWARD_CURVE_INVALID` | Forward curve inputs are not usable |
| `FORWARD_NEGATIVE_FWD_FWD_VARIANCE` | Negative forward-forward variance between two dates |
| `FORWARD_RATE_SOURCE_ANCHOR_MISMATCH` | Forward rate source does not have the correct anchor date |
| `FORWARD_RATE_SOURCE_RANGE` | First forward date is outside the rate source range / Last forward date is outside the rate source range |
| `FORWARD_SIMULATION_DATE_AFTER_RANGE` | Last forward date is outside the rate source range |
| `FORWARD_SIMULATION_DATE_BEFORE_RANGE` | First forward date is outside the rate source range |
| `FORWARD_VOLATILITY_ANCHOR_MISMATCH` | Volatility Rate Source does not have the same anchor date as the Forward Rate Source |
| `FORWARD_VOLATILITY_RATE_SOURCE_MISSING` | Volatility Rate Source has not been set |
| `FORWARD_VOLATILITY_RATE_SOURCE_RANGE` | Volatility Rate Source does not extend to the end of the Forward Rate Source |
| `FX_DOMESTIC_ANCHOR_MISMATCH` | Domestic DRS does not have the correct anchor date |
| `FX_FOREIGN_ANCHOR_MISMATCH` | Foreign DRS does not have the correct anchor date |
| `FX_VOLATILITY_ANCHOR_MISMATCH` | Volatility Rate Source does not have the correct anchor date |
| `HOLDING_PERIOD_CALENDAR_MISSING` | A holding period needs a calendar to count its business days |
| `INSURANCE_AMOUNT_NEGATIVE` | At least one insured amount is negative |
| `INSURANCE_ANCHOR_MISMATCH` | UtilisationDRS and StandbyDRS do not have the same anchor date |
| `INSURANCE_DATES_AMOUNTS_DIMENSION` | Dates and insured amounts do not have the same dimension |
| `INSURANCE_DATES_NOT_INCREASING` | Dates not strictly increasing |
| `INSURANCE_FIRST_DATE` | First insured date preceeds policy start date |
| `INSURANCE_INPUT_DIMENSION` | Input vectors do not have the same dimension |
| `INSURANCE_LAST_DATE` | Last insured date must equal policy maturity date |
| `INSURANCE_MATURITY_BEFORE_START` | Policy maturity is before policy start date |
| `INSURANCE_STANDBY_RANGE` | StandbyDRS does not extend to the policy maturity date |
| `INSURANCE_SUM_NEGATIVE` | Sum insured is negative |
| `INSURANCE_UTILISATION_RANGE` | UtilisationDRS does not extend to the policy maturity date |
| `INTERPOLATOR_DIMENSION_MISMATCH` | Interpolator x and y vectors do not have the same length |
| `INTERPOLATOR_TOO_FEW_POINTS` | Interpolator needs at least two points |
| `INTERPOLATOR_X_NOT_INCREASING` | Interpolator x values are not strictly increasing |
| `MARGIN_COUNTERPARTY_INITIAL_NEGATIVE` | Counterparty Required Initial Margin cannot be negative |
| `MARGIN_COUNTERPARTY_THRESHOLD_NEGATIVE` | Counterparty Threshold cannot be negative |
| `MARGIN_INITIAL_NEGATIVE` | My Required Initial Margin cannot be negative |
| `MARGIN_MINIMUM_TRANSFER_NEGATIVE` | Minimum transfer amount cannot be negative |
| `MARGIN_ROUNDING_NOT_POSITIVE` | Rounding must be strictly positive |
| `MARGIN_THRESHOLD_NEGATIVE` | My Threshold cannot be negative |
| `MARKET_DISCOUNT_RATE_SOURCE_MISSING` | No discounting rate source |
| `MARKET_DISCOUNT_RATE_SOURCE_RANGE` | Discounting rate source does not extend to settlement |
| `MARKET_FIXING_RATE_SOURCE_MISSING` | No fixing rate source |
| `MARKET_FORWARD_RATE_SOURCE_MISSING` | No forward rate source |
| `MARKET_FORWARD_RATE_SOURCE_RANGE` | Contract settlement not in forward rate source range |
| `MARKET_VOLATILITY_RATE_SOURCE_MISSING` | No volatiltiy rate source |
| `MARKET_VOLATILITY_RATE_SOURCE_RANGE` | Contract settlement not in volatility rate source range |
| `MONTE_CARLO_NO_PATHS` | Number of paths must be at least 1 |
| `OPTION_TYPE_INVALID` | Type must be either CALL or PUT |
| `PATH_ANCHOR_MISMATCH` | Rates anchor dates do not match path dates |
| `PATH_GENERATOR_DATES_AFTER_END` | Simulation dates after rate source end |
| `PATH_GENERATOR_DATES_BEFORE_ANCHOR` | Simulation dates before anchor date |
| `PATH_GENERATOR_DISCOUNT_NOT_DETERMINISTIC` | Rates environment does not contain a discounting rate source that rolls deterministically |
| `PATH_GENERATOR_FIXING_NOT_ROLLING` | Rates environment does not contain a fixing rate source that rolls |
| `PATH_GENERATOR_FOREIGN_DISCOUNT_NOT_DETERMINISTIC` | Rates environment does not contain a foreign discounting rate source that rolls  |
| `PATH_GENERATOR_FOREIGN_DISCOUNT_REQUIRED` | FRS requires a foreign interest rate source but rates environment does not contain  |
| `PATH_GENERATOR_FORWARD_NOT_STOCHASTIC` | Rates environment does not contain a forward rate source that rolls stochastically |
| `PATH_GENERATOR_RATES_MISSING` | Rates environment not set |
| `PATH_GENERATOR_VOLATILITY_NOT_DETERMINISTIC` | Rates environment does not contain a volatility rate source that rolls deterministically |
| `PATH_LENGTH_MISMATCH` | number of rates does not match the number of dates |
| `PORTFOLIO_EMPTY` | contains no contracts |
| `PORTFOLIO_STRUCTURE_CONVERSION` | Could not convert the portfolio into a structure |
| `RATES_ENVIRONMENT_NO_RATES` | No rates set |
| `RATE_SOURCE_ANCHOR_AFTER_FINAL` | Anchor date after final date |
| `RATE_SOURCE_EXTRAPOLATION` | Extrapolation past the last pillar is allowed |
| `RATE_SOURCE_FLAT_BACKFILL` | Anchor has no observation; values are flat through the first observation |
| `SIMULATION_ANCHOR_MISMATCH` | First simulation date must be the anchor date |
| `SIMULATION_DATES_EMPTY` | Simulation Dates are empty |
| `SIMULATION_MARKET_INVALID` | Portfolio cannot value against a path rate environment |
| `SPOT_PATHS_EMPTY` | Number of spot paths must be at least 1 |
| `SPOT_PATHS_LENGTH` | Spot paths must be number_of_paths times simulation dates |
| `STRUCTURE_BUILD_FAILED` | Contracts could not be split into capped and uncapped structures |
| `STRUCTURE_CONTRACT_CONVERSION` | Cound not convert Contract into a Structure |
| `STRUCTURE_EMPTY` | Structure contains no contracts |
| `STRUCTURE_NOT_PIECEWISE_LINEAR` | Structure Design Error, Payoff function is not piecewise linear |
| `STRUCTURE_REGCAP_OFFSET` | not all the contracts quality for regulatory capital offset,  |
| `STRUCTURE_UNCAPPED_WING` | has uncapped exposure but maximum volume is not in the (uncapped) wings |
| `STRUCTURE_VOLUME_WITHOUT_EXPOSURE` | Structure Design Error, the structure has no volume but it does have price exposure |
| `SURFACE_INPUTS_INSUFFICIENT` | A two-dimensional interpolator does not have enough x, y or z data |
| `SURFACE_X_DIMENSION` | Surface x vector length does not match the z columns |
| `SURFACE_X_NOT_INCREASING` | Surface x values are not strictly increasing |
| `SURFACE_Y_DIMENSION` | Surface y vector length does not match the z rows |
| `SURFACE_Y_NOT_INCREASING` | Surface y values are not strictly increasing |
| `VOLATILITY_CURVE_CMA_DIMENSION_MISMATCH` | volatility curve cma dimension mismatch |
| `VOLATILITY_CURVE_CMA_INVALID` | Calendar-month-average volatility curve inputs are not usable |
| `VOLATILITY_CURVE_CMA_NEGATIVE_INPUT` | has negative inputs |
| `VOLATILITY_CURVE_DIMENSION_MISMATCH` | volatility curve dimension mismatch |
| `VOLATILITY_CURVE_INVALID` | Volatility curve inputs are not usable |
| `VOLATILITY_CURVE_NEGATIVE_INPUT` | has negative inputs |
| `VOLATILITY_DAY_COUNTER_MISSING` | Day counter not set |
| `VOLATILITY_RATE_SOURCE_ANCHOR_MISMATCH` | Volatility rate source does not have the correct anchor date |
| `VOLATILITY_RATE_SOURCE_RANGE` | First forward date is outside the rate source range / Last forward date is outside the rate source range |
| `VOLATILITY_SURFACE_DATES_NOT_INCREASING` | Input dates not strictly increasing |
| `VOLATILITY_SURFACE_DIMENSION_MISMATCH` | Input vectors and matricies have inconsistent dimension |
| `VOLATILITY_SURFACE_FINAL_BEFORE_ANCHOR` | Final date before anchor date |
| `VOLATILITY_SURFACE_INPUTS_EMPTY` | One of observation dates, moneyness or volatility is empty |
| `VOLATILITY_SURFACE_INTERPOLATOR_MISSING` | Interpolator not set |
| `VOLATILITY_SURFACE_INTERPOLATOR_TYPE` | Interpolator Type must be either Bilinear or Bicubic |
| `VOLATILITY_SURFACE_STRIKES_NOT_INCREASING` | Strike dimension inputs are not strictly increasing |
| `VOLATILITY_SURFACE_TIMES_NOT_INCREASING` | Input times not strictly increasing |
