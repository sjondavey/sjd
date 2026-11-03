#pragma once

#include <string_view>

namespace sjd
{
    namespace issues
    {

        // Stable identifiers stored on a ValidationIssue. The value is the text callers
        // and the Python bindings already compare against.
        inline constexpr std::string_view ASIAN_AVERAGING_DATES_MISSING = "ASIAN_AVERAGING_DATES_MISSING";
        inline constexpr std::string_view ASIAN_FIXING_DATE_MISSING = "ASIAN_FIXING_DATE_MISSING";
        inline constexpr std::string_view ASIAN_MATURITY_NOT_FINAL_AVERAGE =
            "ASIAN_MATURITY_NOT_FINAL_AVERAGE";
        inline constexpr std::string_view ASIAN_SETTLEMENT_BEFORE_FINAL_AVERAGE =
            "ASIAN_SETTLEMENT_BEFORE_FINAL_AVERAGE";
        inline constexpr std::string_view CASHFLOW_MATURITY_NOT_SETTLEMENT =
            "CASHFLOW_MATURITY_NOT_SETTLEMENT";
        inline constexpr std::string_view CONTRACT_EXTRACT_FAILED = "CONTRACT_EXTRACT_FAILED";
        inline constexpr std::string_view CONTRACT_MATURITY_AFTER_SETTLEMENT =
            "CONTRACT_MATURITY_AFTER_SETTLEMENT";
        inline constexpr std::string_view CONTRACT_PREMIUM_DATE_AFTER_SETTLEMENT =
            "CONTRACT_PREMIUM_DATE_AFTER_SETTLEMENT";
        inline constexpr std::string_view CONTRACT_PREMIUM_NEGATIVE = "CONTRACT_PREMIUM_NEGATIVE";
        inline constexpr std::string_view CONTRACT_STRIKE_NEGATIVE = "CONTRACT_STRIKE_NEGATIVE";
        inline constexpr std::string_view CVA_DATES_BEFORE_ANCHOR = "CVA_DATES_BEFORE_ANCHOR";
        inline constexpr std::string_view CVA_LGD_INTERPOLATOR = "CVA_LGD_INTERPOLATOR";
        inline constexpr std::string_view CVA_LGD_RANGE = "CVA_LGD_RANGE";
        inline constexpr std::string_view CVA_PD_ANCHOR_NOT_ZERO = "CVA_PD_ANCHOR_NOT_ZERO";
        inline constexpr std::string_view CVA_PD_INTERPOLATOR = "CVA_PD_INTERPOLATOR";
        inline constexpr std::string_view CVA_PD_NOT_INCREASING = "CVA_PD_NOT_INCREASING";
        inline constexpr std::string_view CVA_PD_RANGE = "CVA_PD_RANGE";
        inline constexpr std::string_view DISCOUNT_CURVE_DIMENSION_MISMATCH =
            "DISCOUNT_CURVE_DIMENSION_MISMATCH";
        inline constexpr std::string_view DISCOUNT_CURVE_INVALID = "DISCOUNT_CURVE_INVALID";
        inline constexpr std::string_view DISCOUNT_CURVE_NEGATIVE_FACTOR = "DISCOUNT_CURVE_NEGATIVE_FACTOR";
        inline constexpr std::string_view DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH =
            "DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH";
        inline constexpr std::string_view DISCOUNT_RATE_SOURCE_RANGE = "DISCOUNT_RATE_SOURCE_RANGE";
        inline constexpr std::string_view FIXING_DATES_AND_RATES_LENGTH = "FIXING_DATES_AND_RATES_LENGTH";
        inline constexpr std::string_view FIXING_DATES_EMPTY = "FIXING_DATES_EMPTY";
        inline constexpr std::string_view FIXING_DATES_NOT_INCREASING = "FIXING_DATES_NOT_INCREASING";
        inline constexpr std::string_view FIXING_FIRST_DATE_MISMATCH = "FIXING_FIRST_DATE_MISMATCH";
        inline constexpr std::string_view FIXING_RATE_OUT_OF_RANGE = "FIXING_RATE_OUT_OF_RANGE";
        inline constexpr std::string_view FOREIGN_DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH =
            "FOREIGN_DISCOUNT_RATE_SOURCE_ANCHOR_MISMATCH";
        inline constexpr std::string_view FORWARD_CURVE_CMA_DIMENSION_MISMATCH =
            "FORWARD_CURVE_CMA_DIMENSION_MISMATCH";
        inline constexpr std::string_view FORWARD_CURVE_CMA_INVALID = "FORWARD_CURVE_CMA_INVALID";
        inline constexpr std::string_view FORWARD_CURVE_DIMENSION_MISMATCH =
            "FORWARD_CURVE_DIMENSION_MISMATCH";
        inline constexpr std::string_view FORWARD_CURVE_INVALID = "FORWARD_CURVE_INVALID";
        inline constexpr std::string_view FORWARD_NEGATIVE_FWD_FWD_VARIANCE =
            "FORWARD_NEGATIVE_FWD_FWD_VARIANCE";
        inline constexpr std::string_view FORWARD_RATE_SOURCE_ANCHOR_MISMATCH =
            "FORWARD_RATE_SOURCE_ANCHOR_MISMATCH";
        inline constexpr std::string_view FORWARD_RATE_SOURCE_RANGE = "FORWARD_RATE_SOURCE_RANGE";
        inline constexpr std::string_view FORWARD_SIMULATION_DATE_AFTER_RANGE =
            "FORWARD_SIMULATION_DATE_AFTER_RANGE";
        inline constexpr std::string_view FORWARD_SIMULATION_DATE_BEFORE_RANGE =
            "FORWARD_SIMULATION_DATE_BEFORE_RANGE";
        inline constexpr std::string_view FORWARD_VOLATILITY_ANCHOR_MISMATCH =
            "FORWARD_VOLATILITY_ANCHOR_MISMATCH";
        inline constexpr std::string_view FORWARD_VOLATILITY_RATE_SOURCE_MISSING =
            "FORWARD_VOLATILITY_RATE_SOURCE_MISSING";
        inline constexpr std::string_view FORWARD_VOLATILITY_RATE_SOURCE_RANGE =
            "FORWARD_VOLATILITY_RATE_SOURCE_RANGE";
        inline constexpr std::string_view FX_DOMESTIC_ANCHOR_MISMATCH = "FX_DOMESTIC_ANCHOR_MISMATCH";
        inline constexpr std::string_view FX_FOREIGN_ANCHOR_MISMATCH = "FX_FOREIGN_ANCHOR_MISMATCH";
        inline constexpr std::string_view FX_VOLATILITY_ANCHOR_MISMATCH = "FX_VOLATILITY_ANCHOR_MISMATCH";
        inline constexpr std::string_view HOLDING_PERIOD_CALENDAR_MISSING = "HOLDING_PERIOD_CALENDAR_MISSING";
        inline constexpr std::string_view INSURANCE_AMOUNT_NEGATIVE = "INSURANCE_AMOUNT_NEGATIVE";
        inline constexpr std::string_view INSURANCE_ANCHOR_MISMATCH = "INSURANCE_ANCHOR_MISMATCH";
        inline constexpr std::string_view INSURANCE_DATES_AMOUNTS_DIMENSION =
            "INSURANCE_DATES_AMOUNTS_DIMENSION";
        inline constexpr std::string_view INSURANCE_DATES_NOT_INCREASING = "INSURANCE_DATES_NOT_INCREASING";
        inline constexpr std::string_view INSURANCE_FIRST_DATE = "INSURANCE_FIRST_DATE";
        inline constexpr std::string_view INSURANCE_INPUT_DIMENSION = "INSURANCE_INPUT_DIMENSION";
        inline constexpr std::string_view INSURANCE_LAST_DATE = "INSURANCE_LAST_DATE";
        inline constexpr std::string_view INSURANCE_MATURITY_BEFORE_START = "INSURANCE_MATURITY_BEFORE_START";
        inline constexpr std::string_view INSURANCE_STANDBY_RANGE = "INSURANCE_STANDBY_RANGE";
        inline constexpr std::string_view INSURANCE_SUM_NEGATIVE = "INSURANCE_SUM_NEGATIVE";
        inline constexpr std::string_view INSURANCE_UTILISATION_RANGE = "INSURANCE_UTILISATION_RANGE";
        inline constexpr std::string_view MARGIN_COUNTERPARTY_INITIAL_NEGATIVE =
            "MARGIN_COUNTERPARTY_INITIAL_NEGATIVE";
        inline constexpr std::string_view MARGIN_COUNTERPARTY_THRESHOLD_NEGATIVE =
            "MARGIN_COUNTERPARTY_THRESHOLD_NEGATIVE";
        inline constexpr std::string_view MARGIN_INITIAL_NEGATIVE = "MARGIN_INITIAL_NEGATIVE";
        inline constexpr std::string_view MARGIN_MINIMUM_TRANSFER_NEGATIVE =
            "MARGIN_MINIMUM_TRANSFER_NEGATIVE";
        inline constexpr std::string_view MARGIN_ROUNDING_NOT_POSITIVE = "MARGIN_ROUNDING_NOT_POSITIVE";
        inline constexpr std::string_view MARGIN_THRESHOLD_NEGATIVE = "MARGIN_THRESHOLD_NEGATIVE";
        inline constexpr std::string_view MARKET_DISCOUNT_RATE_SOURCE_MISSING =
            "MARKET_DISCOUNT_RATE_SOURCE_MISSING";
        inline constexpr std::string_view MARKET_DISCOUNT_RATE_SOURCE_RANGE =
            "MARKET_DISCOUNT_RATE_SOURCE_RANGE";
        inline constexpr std::string_view MARKET_FIXING_RATE_SOURCE_MISSING =
            "MARKET_FIXING_RATE_SOURCE_MISSING";
        inline constexpr std::string_view MARKET_FORWARD_RATE_SOURCE_MISSING =
            "MARKET_FORWARD_RATE_SOURCE_MISSING";
        inline constexpr std::string_view MARKET_FORWARD_RATE_SOURCE_RANGE =
            "MARKET_FORWARD_RATE_SOURCE_RANGE";
        inline constexpr std::string_view MARKET_VOLATILITY_RATE_SOURCE_MISSING =
            "MARKET_VOLATILITY_RATE_SOURCE_MISSING";
        inline constexpr std::string_view MARKET_VOLATILITY_RATE_SOURCE_RANGE =
            "MARKET_VOLATILITY_RATE_SOURCE_RANGE";
        inline constexpr std::string_view MONTE_CARLO_NO_PATHS = "MONTE_CARLO_NO_PATHS";
        inline constexpr std::string_view OPTION_TYPE_INVALID = "OPTION_TYPE_INVALID";
        inline constexpr std::string_view PATH_ANCHOR_MISMATCH = "PATH_ANCHOR_MISMATCH";
        inline constexpr std::string_view PATH_GENERATOR_DATES_AFTER_END = "PATH_GENERATOR_DATES_AFTER_END";
        inline constexpr std::string_view PATH_GENERATOR_DATES_BEFORE_ANCHOR =
            "PATH_GENERATOR_DATES_BEFORE_ANCHOR";
        inline constexpr std::string_view PATH_GENERATOR_DISCOUNT_NOT_DETERMINISTIC =
            "PATH_GENERATOR_DISCOUNT_NOT_DETERMINISTIC";
        inline constexpr std::string_view PATH_GENERATOR_FIXING_NOT_ROLLING =
            "PATH_GENERATOR_FIXING_NOT_ROLLING";
        inline constexpr std::string_view PATH_GENERATOR_FOREIGN_DISCOUNT_NOT_DETERMINISTIC =
            "PATH_GENERATOR_FOREIGN_DISCOUNT_NOT_DETERMINISTIC";
        inline constexpr std::string_view PATH_GENERATOR_FOREIGN_DISCOUNT_REQUIRED =
            "PATH_GENERATOR_FOREIGN_DISCOUNT_REQUIRED";
        inline constexpr std::string_view PATH_GENERATOR_FORWARD_NOT_STOCHASTIC =
            "PATH_GENERATOR_FORWARD_NOT_STOCHASTIC";
        inline constexpr std::string_view PATH_GENERATOR_RATES_MISSING = "PATH_GENERATOR_RATES_MISSING";
        inline constexpr std::string_view PATH_GENERATOR_VOLATILITY_NOT_DETERMINISTIC =
            "PATH_GENERATOR_VOLATILITY_NOT_DETERMINISTIC";
        inline constexpr std::string_view PATH_LENGTH_MISMATCH = "PATH_LENGTH_MISMATCH";
        inline constexpr std::string_view PORTFOLIO_EMPTY = "PORTFOLIO_EMPTY";
        inline constexpr std::string_view PORTFOLIO_STRUCTURE_CONVERSION = "PORTFOLIO_STRUCTURE_CONVERSION";
        inline constexpr std::string_view RATES_ENVIRONMENT_NO_RATES = "RATES_ENVIRONMENT_NO_RATES";
        inline constexpr std::string_view RATE_SOURCE_ANCHOR_AFTER_FINAL = "RATE_SOURCE_ANCHOR_AFTER_FINAL";
        inline constexpr std::string_view RATE_SOURCE_EXTRAPOLATION = "RATE_SOURCE_EXTRAPOLATION";
        inline constexpr std::string_view RATE_SOURCE_FLAT_BACKFILL = "RATE_SOURCE_FLAT_BACKFILL";
        inline constexpr std::string_view SIMULATION_ANCHOR_MISMATCH = "SIMULATION_ANCHOR_MISMATCH";
        inline constexpr std::string_view SIMULATION_DATES_EMPTY = "SIMULATION_DATES_EMPTY";
        inline constexpr std::string_view SIMULATION_MARKET_INVALID = "SIMULATION_MARKET_INVALID";
        inline constexpr std::string_view SPOT_PATHS_EMPTY = "SPOT_PATHS_EMPTY";
        inline constexpr std::string_view SPOT_PATHS_LENGTH = "SPOT_PATHS_LENGTH";
        inline constexpr std::string_view STRUCTURE_CONTRACT_CONVERSION = "STRUCTURE_CONTRACT_CONVERSION";
        inline constexpr std::string_view STRUCTURE_EMPTY = "STRUCTURE_EMPTY";
        inline constexpr std::string_view STRUCTURE_NOT_PIECEWISE_LINEAR = "STRUCTURE_NOT_PIECEWISE_LINEAR";
        inline constexpr std::string_view STRUCTURE_REGCAP_OFFSET = "STRUCTURE_REGCAP_OFFSET";
        inline constexpr std::string_view STRUCTURE_UNCAPPED_WING = "STRUCTURE_UNCAPPED_WING";
        inline constexpr std::string_view STRUCTURE_VOLUME_WITHOUT_EXPOSURE =
            "STRUCTURE_VOLUME_WITHOUT_EXPOSURE";
        inline constexpr std::string_view VOLATILITY_CURVE_CMA_DIMENSION_MISMATCH =
            "VOLATILITY_CURVE_CMA_DIMENSION_MISMATCH";
        inline constexpr std::string_view VOLATILITY_CURVE_CMA_INVALID = "VOLATILITY_CURVE_CMA_INVALID";
        inline constexpr std::string_view VOLATILITY_CURVE_CMA_NEGATIVE_INPUT =
            "VOLATILITY_CURVE_CMA_NEGATIVE_INPUT";
        inline constexpr std::string_view VOLATILITY_CURVE_DIMENSION_MISMATCH =
            "VOLATILITY_CURVE_DIMENSION_MISMATCH";
        inline constexpr std::string_view VOLATILITY_CURVE_INVALID = "VOLATILITY_CURVE_INVALID";
        inline constexpr std::string_view VOLATILITY_CURVE_NEGATIVE_INPUT = "VOLATILITY_CURVE_NEGATIVE_INPUT";
        inline constexpr std::string_view VOLATILITY_DAY_COUNTER_MISSING = "VOLATILITY_DAY_COUNTER_MISSING";
        inline constexpr std::string_view VOLATILITY_RATE_SOURCE_ANCHOR_MISMATCH =
            "VOLATILITY_RATE_SOURCE_ANCHOR_MISMATCH";
        inline constexpr std::string_view VOLATILITY_RATE_SOURCE_RANGE = "VOLATILITY_RATE_SOURCE_RANGE";
        inline constexpr std::string_view VOLATILITY_SURFACE_DATES_NOT_INCREASING =
            "VOLATILITY_SURFACE_DATES_NOT_INCREASING";
        inline constexpr std::string_view VOLATILITY_SURFACE_DIMENSION_MISMATCH =
            "VOLATILITY_SURFACE_DIMENSION_MISMATCH";
        inline constexpr std::string_view VOLATILITY_SURFACE_FINAL_BEFORE_ANCHOR =
            "VOLATILITY_SURFACE_FINAL_BEFORE_ANCHOR";
        inline constexpr std::string_view VOLATILITY_SURFACE_INPUTS_EMPTY = "VOLATILITY_SURFACE_INPUTS_EMPTY";
        inline constexpr std::string_view VOLATILITY_SURFACE_INTERPOLATOR_MISSING =
            "VOLATILITY_SURFACE_INTERPOLATOR_MISSING";
        inline constexpr std::string_view VOLATILITY_SURFACE_INTERPOLATOR_TYPE =
            "VOLATILITY_SURFACE_INTERPOLATOR_TYPE";
        inline constexpr std::string_view VOLATILITY_SURFACE_STRIKES_NOT_INCREASING =
            "VOLATILITY_SURFACE_STRIKES_NOT_INCREASING";
        inline constexpr std::string_view VOLATILITY_SURFACE_TIMES_NOT_INCREASING =
            "VOLATILITY_SURFACE_TIMES_NOT_INCREASING";

    }
}
