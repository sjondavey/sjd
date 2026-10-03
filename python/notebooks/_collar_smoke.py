import math

import sjd
from sjd import examples

anchor = examples.ANCHOR
holidays = examples.sample_calendar()
settlements = [holidays.adjust(examples.add_months(anchor, i)) for i in range(1, 13)]
curve_dates = [anchor, *settlements]
yf = examples.year_fraction
volume = 1_000_000.0
put_moneyness = 0.97
call_moneyness = 1.03

frs, drs, vrs = examples.fx_curves(curve_dates)
environment = sjd.RatesEnvironment(anchor)
environment.set_frs(frs)
environment.set_drs(drs)
environment.set_vrs(vrs)
if not environment.is_ok():
    raise RuntimeError(environment.error_messages())


def make_option(maturity, settlement, strike, option_type, buy_sell):
    option = sjd.EuropeanOption(
        maturity=maturity,
        settlement=settlement,
        strike=strike,
        type=option_type,
        volume=volume,
        premium_payment_date=settlement,
        premium_amount=0.0,
        buy_sell=buy_sell,
    )
    option.validate(environment)
    if not option.is_ok():
        raise ValueError(option.error_messages())
    return option


for settlement in settlements:
    maturity = holidays.advance(settlement, -2)
    atm = frs.get_forward(settlement)
    put_strike = round(atm * put_moneyness, 2)
    call_strike = round(atm * call_moneyness, 2)
    put = make_option(maturity, settlement, put_strike, sjd.PutCall.PUT, sjd.BuySell.BUY)
    call = make_option(maturity, settlement, call_strike, sjd.PutCall.CALL, sjd.BuySell.SELL)
    forward = frs.get_forward(settlement)
    discount_factor = drs.get_discount_factor(settlement)
    standard_deviation = vrs.get_volatility(maturity) * math.sqrt(yf(maturity))
    put_formula = sjd.Black76Put(forward, put_strike, standard_deviation, discount_factor)
    call_formula = sjd.Black76Call(forward, call_strike, standard_deviation, discount_factor)
    if not put_formula.is_ok():
        raise ValueError(put_formula.error_message())
    if not call_formula.is_ok():
        raise ValueError(call_formula.error_message())
    expected_put = put_formula.premium() * volume
    expected_call = -call_formula.premium() * volume
    assert math.isclose(put.get_value(environment), expected_put, rel_tol=1e-12)
    assert math.isclose(call.get_value(environment), expected_call, rel_tol=1e-12)
    assert put_strike < atm < call_strike

print("All 12 collars match Black-76 (purchased put + sold call).")
