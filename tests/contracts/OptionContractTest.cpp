// to use the boost::math::isnan(...) function

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/all.hpp>
#include <sjd/contracts/AsianOptionContract.h>
#include <sjd/contracts/EuropeanForwardContract.h>
#include <sjd/contracts/EuropeanOptionContract.h>
#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;

BOOST_AUTO_TEST_SUITE(OptionContractTest)

std::shared_ptr<EuropeanOption> getDefaultEuropeanOption(Date anchorDate);

std::shared_ptr<EuropeanOption> getDefaultEuropeanOption(Date anchorDate)
{
    NullCalendar nullCalendar;
    Date optionMaturity = nullCalendar.advance(anchorDate, 2, Years);
    Date optionSettlement = nullCalendar.advance(optionMaturity, 2, Days);
    double strike = 100;
    PutCall type = PutCall::PUT;
    double defPremium = 3.4;
    double volume = 10000;

    return std::make_shared<EuropeanOption>(optionMaturity, optionSettlement, strike, type, volume,
                                            optionSettlement, defPremium, BuySell::BUY);
}
BOOST_AUTO_TEST_CASE(testBuySell)
{
    BOOST_TEST_MESSAGE("Testing OptionContract ...");
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanOption> base = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<EuropeanOption> contract = std::make_shared<EuropeanOption>(
        base->getMaturityDate(), base->getSettlementDate(), base->getStrike(), base->getType(),
        base->getVolume(), base->getPremiumPaymentDate(), base->getPremiumAmount(), BuySell::SELL);
    BOOST_CHECK(contract->getBuySell() == BuySell::SELL);
}
BOOST_AUTO_TEST_CASE(testStrike)
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanOption> base = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<EuropeanOption> contract = std::make_shared<EuropeanOption>(
        base->getMaturityDate(), base->getSettlementDate(), -1.0, base->getType(), base->getVolume(),
        base->getPremiumPaymentDate(), base->getPremiumAmount(), base->getBuySell());
    contract->validate();
    BOOST_CHECK(!contract->isOK());
    BOOST_CHECK_SMALL(contract->getStrike() + 1.0, EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testVolume)
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanOption> base = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<EuropeanOption> contract = std::make_shared<EuropeanOption>(
        base->getMaturityDate(), base->getSettlementDate(), base->getStrike(), base->getType(), -1.0,
        base->getPremiumPaymentDate(), base->getPremiumAmount(), base->getBuySell());
    BOOST_CHECK(contract->isOK());
    BOOST_CHECK_SMALL(contract->getVolume() - 1.0, EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testMaturityDate)
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanOption> base = getDefaultEuropeanOption(anchorDate);
    NullCalendar nullCalendar;
    Date optionMaturity = nullCalendar.advance(anchorDate, 3, Years);
    std::shared_ptr<EuropeanOption> contract = std::make_shared<EuropeanOption>(
        optionMaturity, base->getSettlementDate(), base->getStrike(), base->getType(), base->getVolume(),
        base->getPremiumPaymentDate(), base->getPremiumAmount(), base->getBuySell());
    contract->validate();
    BOOST_CHECK(!contract->isOK());
    BOOST_CHECK(contract->getMaturityDate() == optionMaturity);
}
BOOST_AUTO_TEST_CASE(testPremiumAmount)
{
    Date anchorDate(1, Jan, 2013);
    std::shared_ptr<EuropeanOption> base = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<EuropeanOption> contract = std::make_shared<EuropeanOption>(
        base->getMaturityDate(), base->getSettlementDate(), base->getStrike(), base->getType(),
        base->getVolume(), base->getPremiumPaymentDate(), -10.0, base->getBuySell());
    BOOST_CHECK_SMALL(contract->getPremiumAmount() - 10.0, EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testType)
{
    Date anchorDate(1, Jan, 2013);
    GenerateRates gr = GenerateRates();

    std::shared_ptr<EuropeanOption> put = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<RatesEnvironment> re =
        gr.getRatesEnvironmentForEuropeanOption(anchorDate, vector<Date>(0));
    BOOST_CHECK(put->getType() == PutCall::PUT);
    double valuePut = put->getValue(re);
    std::shared_ptr<EuropeanOption> call = std::make_shared<EuropeanOption>(
        put->getMaturityDate(), put->getSettlementDate(), put->getStrike(), PutCall::CALL, put->getVolume(),
        put->getPremiumPaymentDate(), put->getPremiumAmount(), put->getBuySell());
    BOOST_CHECK(call->getType() == PutCall::CALL);
    double valueCall = call->getValue(re);
    BOOST_CHECK(abs(valuePut - valueCall) > EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testPutCallParity)
{
    Date anchorDate(1, Jan, 2013);
    GenerateRates gr = GenerateRates();

    std::shared_ptr<EuropeanOption> put = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<EuropeanOption> call = std::make_shared<EuropeanOption>(
        put->getMaturityDate(), put->getSettlementDate(), put->getStrike(), PutCall::CALL, put->getVolume(),
        put->getPremiumPaymentDate(), put->getPremiumAmount(), BuySell::SELL);
    std::shared_ptr<RatesEnvironment> re =
        gr.getRatesEnvironmentForEuropeanOption(anchorDate, vector<Date>(0));

    EuropeanForward forward(put->getMaturityDate(), put->getSettlementDate(), put->getStrike(),
                            put->getVolume(), BuySell::SELL);
    BOOST_CHECK_SMALL(put->getValue(re) + call->getValue(re) - forward.getValue(re), 1e-10);
}
BOOST_AUTO_TEST_CASE(testValue)
{
    Date anchorDate(1, Jan, 2013);
    GenerateRates gr = GenerateRates();

    DayCounter dayCounter = gr.defaultDayCounter;
    ;
    Calendar nullCalendar = gr.defaultCalendar;

    Date tPlus2 = nullCalendar.advance(anchorDate, 2, Days);
    Date optionMaturity = nullCalendar.advance(anchorDate, 2, Years);
    Date optionSettlement = nullCalendar.advance(optionMaturity, 2, Days);

    vector<Date> manditoryDates;
    manditoryDates.push_back(tPlus2);
    manditoryDates.push_back(optionMaturity);
    manditoryDates.push_back(optionSettlement);

    std::shared_ptr<RatesEnvironment> re =
        gr.getRatesEnvironmentForEuropeanOption(anchorDate, manditoryDates);

    double strike = 100;
    PutCall type = PutCall::PUT;
    double defPremium = 3.4;

    EuropeanOption contract(optionMaturity, optionSettlement, strike, type, 1.0, anchorDate, 0.0,
                            BuySell::BUY);
    EuropeanOption contract2(optionMaturity, optionSettlement, strike, type, 2.0, anchorDate, 0.0,
                             BuySell::BUY);
    EuropeanOption deferredPremiumContract(optionMaturity, optionSettlement, strike, type, 1,
                                           optionSettlement, defPremium, BuySell::BUY);
    EuropeanOption deferredPremiumContract2(optionMaturity, optionSettlement, strike, type, 2,
                                            optionSettlement, defPremium, BuySell::BUY);

    double yts = dayCounter.yearFraction(anchorDate, optionSettlement);
    double fwd = re->getFRS()->getForward(optionSettlement);
    // Maturity is a pillar, so the input volatility is recovered. The premium uses
    // the curve's standard deviation, which is that volatility times sqrt(time).
    BOOST_CHECK_SMALL(re->getVRS()->getVolatility(optionMaturity) - gr.defaultVolatiltiy,
                      EPSILON_FOR_DOUBLES);
    double sd = re->getVRS()->getStandardDeviation(optionMaturity, strike, fwd);
    Black76Put simplePut(fwd, strike, sd, exp(-gr.defaultNACCRate * yts));
    double pvDefPremium = defPremium * re->getDRS()->getDiscountFactor(optionSettlement);

    // 1) Before maturity
    contract.validate();
    BOOST_CHECK_SMALL(simplePut.getPremium() - contract.getOptionValue(re), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(simplePut.getPremium() - contract.getValue(re), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(contract2.getOptionValue(re) - 2 * contract.getOptionValue(re), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(contract2.getValue(re) - 2 * contract.getValue(re), EPSILON_FOR_DOUBLES);

    BOOST_CHECK_SMALL(deferredPremiumContract2.getOptionValue(re) -
                          2 * deferredPremiumContract.getOptionValue(re),
                      EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(deferredPremiumContract2.getOptionValue(re) -
                          2 * (pvDefPremium + deferredPremiumContract.getValue(re)),
                      EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(simplePut.getPremium() - deferredPremiumContract.getOptionValue(re),
                      EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(simplePut.getPremium() - pvDefPremium - deferredPremiumContract.getValue(re),
                      EPSILON_FOR_DOUBLES);

    // 3) After settlement
    Date dayAfterSettlement = nullCalendar.advance(optionSettlement, 1, Days);
    manditoryDates.clear();
    re = gr.getRatesEnvironmentForEuropeanOption(dayAfterSettlement, manditoryDates);
    BOOST_CHECK_SMALL(contract2.getOptionValue(re), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(deferredPremiumContract2.getOptionValue(re), EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testGetPayoffAfterMaturity)
{
    Actual365Fixed dayCounter;
    NullCalendar nullCalendar;

    Date anchorDate(1, Jan, 2013);

    std::shared_ptr<EuropeanOption> option = getDefaultEuropeanOption(anchorDate);
    std::shared_ptr<EuropeanOption> option2 = std::make_shared<EuropeanOption>(
        option->getMaturityDate(), option->getSettlementDate(), option->getStrike(), PutCall::CALL,
        option->getVolume(), option->getPremiumPaymentDate(), option->getPremiumAmount(),
        option->getBuySell());

    double v1 = option->getVolume();
    double p1 = option->getPremiumAmount();
    double k1 = option->getStrike();
    double sign1 = (option->getBuySell() == BuySell::BUY ? 1 : -1);

    double maturitySpot = k1 - 10;
    BOOST_CHECK_SMALL(option->getPayoff(maturitySpot) - (k1 - maturitySpot - p1) * v1 * sign1, 1e-14);
    maturitySpot = k1 + 10;
    BOOST_CHECK_SMALL(option->getPayoff(maturitySpot) - (-p1) * v1 * sign1, 1e-14);
    BOOST_CHECK_SMALL(option2->getPayoff(maturitySpot) - (maturitySpot - k1 - p1) * v1 * sign1, 1e-14);
}
BOOST_AUTO_TEST_CASE(testEuropeanOptionWithMoneynessVol)
{
    BOOST_TEST_MESSAGE("Testing European Option with moneyness vol ...");

    std::shared_ptr<RatesEnvironment> re;
    GenerateRates gr = GenerateRates();

    DayCounter dayCounter = gr.defaultDayCounter;
    Calendar nullCalendar = gr.defaultCalendar;

    Date anchorDate = gr.defaultAnchorDate;
    Date tPlus2 = nullCalendar.advance(anchorDate, 2, Days);
    Date optionMaturity = nullCalendar.advance(anchorDate, 1, Years);
    Date optionSettlement = nullCalendar.advance(optionMaturity, 2, Days);

    vector<Date> manditoryDates;
    manditoryDates.push_back(tPlus2);
    manditoryDates.push_back(optionMaturity);
    manditoryDates.push_back(optionSettlement);

    double spot = gr.defaultSpot;
    double naccRFR = gr.defaultNACCRate;
    double naccIRDiff = gr.defaultNACCIRDiff;

    std::shared_ptr<DiscountCurve> dc = gr.getDefaultDiscountCurve(anchorDate);
    std::shared_ptr<ForwardCurveDeterministic> fc =
        gr.getSimpleForwardCurve(dayCounter, nullCalendar, anchorDate, 10, spot, naccIRDiff, manditoryDates);
    std::shared_ptr<VolatilitySurfaceMoneyness> vs = gr.getDefaultMoneynessSurface(anchorDate);
    re = std::make_shared<RatesEnvironment>(anchorDate);
    re->setDRS(dc);
    re->setFRS(fc);
    re->setVRS(vs);

    double strike = 100;
    PutCall type = PutCall::PUT;
    double defPremium = 3.4;
    double volume = 2.5;

    EuropeanOption contract(optionMaturity, optionSettlement, strike, type, volume, anchorDate, 0.0,
                            BuySell::BUY);
    EuropeanOption deferredPremiumContract(optionMaturity, optionSettlement, strike, type, volume,
                                           optionSettlement, defPremium, BuySell::BUY);

    [[maybe_unused]] double yte = dayCounter.yearFraction(anchorDate, optionMaturity);
    [[maybe_unused]] double yts = dayCounter.yearFraction(anchorDate, optionSettlement);
    double fwd = fc->getForward(optionSettlement);
    double df = dc->getDiscountFactor(optionSettlement);
    double sd = vs->getStandardDeviation(optionMaturity, strike, fwd);

    Black76Put simplePut(fwd, strike, sd, df);
    double pvDefPremium = defPremium * df;

    // 1) Before maturity
    BOOST_CHECK_SMALL(simplePut.getPremium() * volume - contract.getOptionValue(re), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(simplePut.getPremium() * volume - contract.getValue(re), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(simplePut.getPremium() * volume - deferredPremiumContract.getOptionValue(re),
                      EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL((simplePut.getPremium() - pvDefPremium) * volume - deferredPremiumContract.getValue(re),
                      EPSILON_FOR_DOUBLES);

    // 3) After settlement
    Date dayAfterSettlement = nullCalendar.advance(optionSettlement, 1, Days);
    re = std::make_shared<RatesEnvironment>(dayAfterSettlement);
    manditoryDates.clear();
    dc = gr.getDiscountCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, naccRFR);
    fc = gr.getSimpleForwardCurve(dayCounter, nullCalendar, dayAfterSettlement, 10, spot, naccIRDiff);
    vs = gr.getDefaultMoneynessSurface(dayAfterSettlement);
    re->setDRS(dc);
    re->setFRS(fc);
    re->setVRS(vs);

    BOOST_CHECK_SMALL(contract.getOptionValue(re), EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(deferredPremiumContract.getOptionValue(re), EPSILON_FOR_DOUBLES);
}

BOOST_AUTO_TEST_SUITE_END()
