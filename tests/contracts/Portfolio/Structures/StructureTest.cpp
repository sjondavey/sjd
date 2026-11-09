// to use the boost::math::isnan(...) function

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/all.hpp>
#include <sjd/contracts/AsianForwardContract.h>
#include <sjd/contracts/AsianOptionContract.h>
#include <sjd/contracts/Cashflow.h>
#include <sjd/contracts/EuropeanForwardContract.h>
#include <sjd/contracts/EuropeanOptionContract.h>
#include <sjd/contracts/Portfolio/Structures/CemNettingSet.h>
#include <support/GenerateContracts.h>
#include <support/GenerateRatesEnvironments.h>
#include <support/TestingParameters.h>

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;

BOOST_AUTO_TEST_SUITE(StructureTest)

AssetClass getDefaultRegCapUnderlyingFactors();

AssetClass getDefaultRegCapUnderlyingFactors()
{
    return AssetClass::Commodity;
}
BOOST_AUTO_TEST_CASE(testGradient)
{
    BOOST_TEST_MESSAGE("Testing Structures ...");

    Date anchorDate = Date(27, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 12300;

    std::shared_ptr<SingleInstrumentContract> call1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::BUY, PutCall::CALL, 0);
    std::shared_ptr<SingleInstrumentContract> call2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 120, 2 * volume, BuySell::SELL, PutCall::CALL, 0);
    std::shared_ptr<SingleInstrumentContract> call3 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 160, volume, BuySell::BUY, PutCall::CALL, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    instruments.push_back(call1);
    instruments.push_back(call2);
    instruments.push_back(call3);

    double g = CemNettingSet::gradient(instruments, 60, 70);
    BOOST_CHECK_SMALL(g, EPSILON_FOR_DOUBLES);

    g = CemNettingSet::gradient(instruments, 80, 85);
    BOOST_CHECK_SMALL(g - volume, EPSILON_FOR_DOUBLES);

    g = CemNettingSet::gradient(instruments, 120, 130);
    BOOST_CHECK_SMALL(g + volume, EPSILON_FOR_DOUBLES);

    g = CemNettingSet::gradient(instruments, 160, 170);
    BOOST_CHECK_SMALL(g, EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testIsUncappedOnTheLeft)
{
    Date anchorDate = Date(27, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 12300;

    std::shared_ptr<SingleInstrumentContract> put1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::BUY, PutCall::PUT, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    instruments.push_back(put1);

    double effectiveGradient;
    bool uncapped = CemNettingSet::isUncappedOnTheLeft(instruments, effectiveGradient);
    BOOST_CHECK(uncapped);
    BOOST_CHECK_SMALL(effectiveGradient - (-volume), volume * EPSILON_FOR_DOUBLES);
    double g = CemNettingSet::gradient(instruments, 40, 50);
    BOOST_CHECK_SMALL(effectiveGradient - g, volume * EPSILON_FOR_DOUBLES);

    std::shared_ptr<SingleInstrumentContract> put2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 70, volume, BuySell::SELL, PutCall::PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    uncapped = CemNettingSet::isUncappedOnTheLeft(instruments, effectiveGradient);
    BOOST_CHECK(!uncapped);
    BOOST_CHECK_SMALL(effectiveGradient, volume * EPSILON_FOR_DOUBLES);
    g = CemNettingSet::gradient(instruments, 40, 50);
    BOOST_CHECK_SMALL(effectiveGradient - g, volume * EPSILON_FOR_DOUBLES);

    put2 = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 70, volume,
                                                       BuySell::BUY, PutCall::PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    uncapped = CemNettingSet::isUncappedOnTheLeft(instruments, effectiveGradient);
    BOOST_CHECK(uncapped);
    BOOST_CHECK_SMALL(effectiveGradient - (-2 * volume), volume * EPSILON_FOR_DOUBLES);
    g = CemNettingSet::gradient(instruments, 40, 50);
    BOOST_CHECK_SMALL(effectiveGradient - g, volume * EPSILON_FOR_DOUBLES);

    std::shared_ptr<SingleInstrumentContract> put3 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 60, volume, BuySell::SELL, PutCall::PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    instruments.push_back(put3);
    uncapped = CemNettingSet::isUncappedOnTheLeft(instruments, effectiveGradient);
    BOOST_CHECK(uncapped);
    BOOST_CHECK_SMALL(effectiveGradient - (-volume), volume * EPSILON_FOR_DOUBLES);
    g = CemNettingSet::gradient(instruments, 40, 50);
    BOOST_CHECK_SMALL(effectiveGradient - g, volume * EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testGetContractsWithUncappedExposureOnTheLeft)
{
    Date anchorDate = Date(27, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 12300;

    std::shared_ptr<SingleInstrumentContract> put1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::BUY, PutCall::PUT, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    instruments.push_back(put1);

    double commonStrike;
    vector<std::shared_ptr<SingleInstrumentContract>> uncappedContracts;
    uncappedContracts = CemNettingSet::getContractsWithUncappedExposureOnTheLeft(instruments, commonStrike);
    BOOST_CHECK(uncappedContracts.size() == 1);
    BOOST_CHECK(uncappedContracts[0] == put1);

    std::shared_ptr<SingleInstrumentContract> put2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 70, volume, BuySell::SELL, PutCall::PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    uncappedContracts = CemNettingSet::getContractsWithUncappedExposureOnTheLeft(instruments, commonStrike);
    BOOST_CHECK(uncappedContracts.size() == 1);
    BOOST_CHECK(uncappedContracts[0] == put1);

    put2 = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 70, volume,
                                                       BuySell::BUY, PutCall::PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    uncappedContracts = CemNettingSet::getContractsWithUncappedExposureOnTheLeft(instruments, commonStrike);
    BOOST_CHECK(uncappedContracts.size() == 1);
    BOOST_CHECK(uncappedContracts[0] == put2);

    put2 = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 80, volume,
                                                       BuySell::BUY, PutCall::PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    uncappedContracts = CemNettingSet::getContractsWithUncappedExposureOnTheLeft(instruments, commonStrike);
    BOOST_CHECK(uncappedContracts.size() == 2);
    BOOST_CHECK(uncappedContracts[0] == put1);
    BOOST_CHECK(uncappedContracts[1] == put2);
}
BOOST_AUTO_TEST_CASE(testQualifyForRegulatoryCapitalOffset)
{
    Date anchorDate = Date(27, Jan, 2013);
    int yearsToSettlement = 2;
    std::shared_ptr<SingleInstrumentContract> fwd =
        GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToSettlement, 112, 45, BuySell::BUY);
    std::shared_ptr<SingleInstrumentContract> option = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 115, 45, BuySell::SELL, PutCall::CALL, 0);
    std::shared_ptr<SingleInstrumentContract> cf = GenerateContracts::getDefaultFixedCashflowWithPremium(
        anchorDate, yearsToSettlement, 5, BuySell::SELL);

    string errorMsg;
    BOOST_CHECK(CemNettingSet::qualifyForRegulatoryCapitalOffset(fwd, cf, errorMsg));
    BOOST_CHECK(CemNettingSet::qualifyForRegulatoryCapitalOffset(fwd, option, errorMsg));

    option = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement + 1, 115, 45,
                                                         BuySell::SELL, PutCall::CALL, 0);
    BOOST_CHECK(!CemNettingSet::qualifyForRegulatoryCapitalOffset(fwd, option, errorMsg));

    int numberOfAveragingDates = 30;
    double volume = 47;
    std::shared_ptr<SingleDerivativeContract> asianOption =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingDates, 90,
                                                 volume, BuySell::BUY, PutCall::CALL, 10, true);
    BOOST_CHECK(CemNettingSet::qualifyForRegulatoryCapitalOffset(cf, asianOption, errorMsg));
    BOOST_CHECK(!CemNettingSet::qualifyForRegulatoryCapitalOffset(fwd, asianOption, errorMsg));

    std::shared_ptr<SingleDerivativeContract> asianFwd = GenerateContracts::getDefaultAsianForward(
        anchorDate, yearsToSettlement, numberOfAveragingDates, 90, volume, BuySell::BUY);
    BOOST_CHECK(CemNettingSet::qualifyForRegulatoryCapitalOffset(asianFwd, asianOption, errorMsg));

    std::shared_ptr<SingleDerivativeContract> asianFwd2 = GenerateContracts::getDefaultAsianForward(
        anchorDate, yearsToSettlement, numberOfAveragingDates - 1, 90, volume, BuySell::BUY);
    BOOST_CHECK(!CemNettingSet::qualifyForRegulatoryCapitalOffset(asianFwd, asianFwd2, errorMsg));
}
BOOST_AUTO_TEST_CASE(testMoveContracts)
{
    Date anchorDate = Date(27, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 12300;

    std::shared_ptr<SingleInstrumentContract> put1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::BUY, PutCall::PUT, 0);
    std::shared_ptr<SingleInstrumentContract> put2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 70, volume, BuySell::BUY, PutCall::PUT, 0);
    std::shared_ptr<SingleInstrumentContract> call1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 120, volume, BuySell::BUY, PutCall::CALL, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> inputInstruments, identifiedInstruments,
        outputInstruments;
    inputInstruments.push_back(put1);
    inputInstruments.push_back(put2);
    inputInstruments.push_back(call1);

    identifiedInstruments.push_back(call1);

    bool sucess = CemNettingSet::moveContracts(identifiedInstruments, inputInstruments, outputInstruments);
    BOOST_REQUIRE(sucess);
    BOOST_CHECK(inputInstruments.size() == 2);
    BOOST_CHECK(outputInstruments.size() == 1);
    BOOST_CHECK(outputInstruments[0] == call1);

    sucess = CemNettingSet::moveContracts(identifiedInstruments, inputInstruments, outputInstruments);
    BOOST_REQUIRE(!sucess);

    identifiedInstruments.clear();
    identifiedInstruments.push_back(put1);
    sucess = CemNettingSet::moveContracts(identifiedInstruments, inputInstruments, outputInstruments);
    BOOST_REQUIRE(sucess);
    BOOST_CHECK(inputInstruments.size() == 1);
    BOOST_CHECK(outputInstruments.size() == 2);
    BOOST_CHECK(outputInstruments[1] == put1);
}
BOOST_AUTO_TEST_CASE(testSettlementDate)
{

    Date anchorDate = Date(27, Jan, 2013);
    int yearsToMaturity = 2;
    std::shared_ptr<ForwardContract> fwd =
        GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToMaturity, 112, 45, BuySell::BUY);
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 5, BuySell::SELL);
    AssetClass factors = getDefaultRegCapUnderlyingFactors();

    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    instruments.push_back(fwd);
    instruments.push_back(cf);

    CemNettingSet structure(instruments, factors);

    Date sd1 = fwd->getSettlementDate();
    Date sd2 = structure.getSettlementDate();

    BOOST_REQUIRE(structure.isOK());
    BOOST_CHECK(sd1 == sd2);
}
BOOST_AUTO_TEST_CASE(testCashFlowN)
{

    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForCashflow(anchorDate, vector<Date>(0));

    int yearsToMaturity = 2;
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 5, BuySell::BUY);
    AssetClass factors = getDefaultRegCapUnderlyingFactors();

    // std::shared_ptr<DayCounter> dayCounter = std::make_shared<Actual365Fixed>();
    Calendar nullCalendar = gr.defaultCalendar;
    // std::shared_ptr<DiscountCurve> dc = gr.getDiscountCurve(dayCounter, nullCalendar, anchorDate, 5, 0.05);
    double ngr = .36;
    set<Date> forwardDates;
    forwardDates.insert(nullCalendar.advance(anchorDate, 1, Years));
    forwardDates.insert(nullCalendar.advance(anchorDate, 2, Years));
    vector<double> forwardSpots1 = vector<double>(2, 80.0);
    vector<double> forwardSpots2 = vector<double>(2, 100.0);
    vector<double> forwardNgrs = vector<double>(2, ngr);

    // a) no premium
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    instruments.push_back(cf);
    std::shared_ptr<CemNettingSet> structure = std::make_shared<CemNettingSet>(instruments, factors);

    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_REQUIRE(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() - cf->getVolume(), EPSILON_FOR_DOUBLES);
    double expectedMtM = cf->getValue(re);
    BOOST_CHECK_SMALL(structure->getValue(re) - expectedMtM, EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - cf->getVolume(), EPSILON_FOR_DOUBLES);

    // b) premium
    cf = GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 5, BuySell::BUY);
    instruments.clear();
    instruments.push_back(cf);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_REQUIRE(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() - (cf->getVolume() - cf->getPremiumAmount()), EPSILON_FOR_DOUBLES);
    expectedMtM = cf->getValue(re);
    BOOST_CHECK_SMALL(structure->getValue(re) - expectedMtM, EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - (cf->getVolume() - cf->getPremiumAmount()),
                      EPSILON_FOR_DOUBLES);

    // c) sold cashflow
    cf = GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 5, BuySell::SELL);
    instruments.clear();
    instruments.push_back(cf);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_REQUIRE(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() + ((cf->getVolume() - cf->getPremiumAmount())),
                      EPSILON_FOR_DOUBLES);
    expectedMtM = cf->getValue(re);
    BOOST_CHECK_SMALL(structure->getValue(re) - expectedMtM, EPSILON_FOR_DOUBLES);
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) + (cf->getVolume() - cf->getPremiumAmount()),
                      EPSILON_FOR_DOUBLES);

    // d multiple cash flows
    cf = GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 5, BuySell::BUY);
    std::shared_ptr<FixedCashflow> cf2 =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 2, BuySell::SELL);
    std::shared_ptr<FixedCashflow> cf3 =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 1, BuySell::BUY);
    instruments.clear();
    instruments.push_back(cf);
    instruments.push_back(cf2);
    instruments.push_back(cf3);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_REQUIRE(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() - (5 - 2 + 1), EPSILON_FOR_DOUBLES);
}
BOOST_AUTO_TEST_CASE(testEuropeanForward)
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re =
        gr.getRatesEnvironmentForEuropeanForward(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    Calendar nullCalendar = gr.defaultCalendar;
    Date sd = nullCalendar.advance(anchorDate, 2, Years);
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, BuySell::BUY);
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<CemNettingSet> structure;
    AssetClass factors = getDefaultRegCapUnderlyingFactors();
    double volume = 10000;
    double spot = re->getFRS()->getForward(re->getAnchorDate());
    double ngr = .36;

    // a) purchased forward
    std::shared_ptr<EuropeanForward> ef = GenerateContracts::getDefaultEuropeanForward(
        anchorDate, yearsToSettlement, 100, volume, BuySell::BUY);
    instruments.clear();
    instruments.push_back(ef);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());
    double expectedMtM = structure->getValue(re);
    double expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    double expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      EPSILON_FOR_DOUBLES * volume);

    // b) Forward with premium
    instruments.clear();
    instruments.push_back(ef);
    instruments.push_back(cf);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());

    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      EPSILON_FOR_DOUBLES * volume);

    // c) Buy and sell forward - lock in profit
    std::shared_ptr<EuropeanForward> ef2 = GenerateContracts::getDefaultEuropeanForward(
        anchorDate, yearsToSettlement, 120, volume, BuySell::SELL);
    instruments.clear();
    instruments.push_back(ef);
    instruments.push_back(ef2);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() - (120 - 100) * volume, EPSILON_FOR_DOUBLES);

    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    expectedMtMPlusAddOn =
        (120 - 100) * volume; // + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      EPSILON_FOR_DOUBLES * volume);

    // d) Buy and sell forward - lock in loss
    ef2 = GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToSettlement, 80, volume,
                                                       BuySell::SELL);
    instruments.clear();
    instruments.push_back(ef);
    instruments.push_back(ef2);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(!structure->getHasPriceExposure());
    BOOST_CHECK_SMALL(structure->getCap() - (80 - 100) * volume, EPSILON_FOR_DOUBLES * volume);
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    expectedMtMPlusAddOn = (80 - 100) * volume;
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      EPSILON_FOR_DOUBLES * volume);

    // e) Portfolio of 3 forwards
    std::shared_ptr<EuropeanForward> ef3 = GenerateContracts::getDefaultEuropeanForward(
        anchorDate, yearsToSettlement, 120, volume, BuySell::SELL);
    instruments.clear();
    instruments.push_back(ef);
    instruments.push_back(ef2);
    instruments.push_back(ef3);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());

    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      2 * EPSILON_FOR_DOUBLES * volume);
}
BOOST_AUTO_TEST_CASE(testAsianforward)
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianForward(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    Calendar nullCalendar = gr.defaultCalendar;
    Date sd = nullCalendar.advance(anchorDate, yearsToSettlement, Years);
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, BuySell::BUY);
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<CemNettingSet> structure;
    double volume = 10000;
    AssetClass factors = getDefaultRegCapUnderlyingFactors();

    double spot = re->getFRS()->getForward(anchorDate);
    double ngr = .23;
    set<Date> forwardDates;
    forwardDates.insert(nullCalendar.advance(anchorDate, 1, Years));
    forwardDates.insert(sd);
    vector<double> forwardSpots1 = vector<double>(2, 80.0);
    vector<double> forwardSpots2 = vector<double>(2, 100.0);
    vector<double> forwardNgrs = vector<double>(2, ngr);

    // a) purchased forward
    int numberOfAveragingPoints = 30;
    std::shared_ptr<AsianForward> af = GenerateContracts::getDefaultAsianForward(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 100, volume, BuySell::BUY);
    instruments.clear();
    instruments.push_back(af);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());

    double expectedMtM = structure->getValue(re);
    double expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    double expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      EPSILON_FOR_DOUBLES * volume);

    // b) Forward with premium
    instruments.clear();
    instruments.push_back(af);
    instruments.push_back(cf);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      EPSILON_FOR_DOUBLES * volume);

    // c) Buy and sell forward - lock in profit
    std::shared_ptr<AsianForward> af2 = GenerateContracts::getDefaultAsianForward(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 120, volume, BuySell::SELL);
    instruments.clear();
    instruments.push_back(af);
    instruments.push_back(af2);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(!structure->getHasPriceExposure());
    BOOST_CHECK_SMALL(structure->getCap() - (120 - 100) * volume, EPSILON_FOR_DOUBLES);
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    expectedMtMPlusAddOn =
        (120 - 100) * volume; // + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      EPSILON_FOR_DOUBLES * volume);

    // d) Buy and sell forward - lock in loss
    af2 = GenerateContracts::getDefaultAsianForward(anchorDate, yearsToSettlement, numberOfAveragingPoints,
                                                    80, volume, BuySell::SELL);
    instruments.clear();
    instruments.push_back(af);
    instruments.push_back(af2);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(!structure->getHasPriceExposure());
    BOOST_CHECK_SMALL(structure->getCap() - (80 - 100) * volume, EPSILON_FOR_DOUBLES * volume);
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    expectedMtMPlusAddOn = (80 - 100) * volume;
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      EPSILON_FOR_DOUBLES * volume);

    // e) Portfolio of 3 forwards
    std::shared_ptr<AsianForward> af3 = GenerateContracts::getDefaultAsianForward(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 120, volume, BuySell::SELL);
    instruments.clear();
    instruments.push_back(af);
    instruments.push_back(af2);
    instruments.push_back(af3);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      2 * EPSILON_FOR_DOUBLES * volume);
}
BOOST_AUTO_TEST_CASE(testEuropeanOption)
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re =
        gr.getRatesEnvironmentForEuropeanOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    Calendar nullCalendar = gr.defaultCalendar;
    Date sd = nullCalendar.advance(anchorDate, yearsToSettlement, Years);
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, BuySell::BUY);
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<CemNettingSet> structure;
    double premium = 3.4;
    double volume = 10000;
    AssetClass factors = getDefaultRegCapUnderlyingFactors();
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    std::shared_ptr<VolatilityCurve> vc;
    double spot = re->getFRS()->getForward(anchorDate);
    double ngr = .23;
    set<Date> forwardDates;
    forwardDates.insert(nullCalendar.advance(anchorDate, 1, Years));
    forwardDates.insert(sd);
    vector<double> forwardSpots1 = vector<double>(2, 80.0);
    vector<double> forwardSpots2 = vector<double>(2, 100.0);
    vector<double> forwardNgrs = vector<double>(2, ngr);

    // a) purchased option
    std::shared_ptr<EuropeanOption> purchasedPutDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::BUY, PutCall::PUT, premium, true);
    std::shared_ptr<EuropeanOption> purchasedPutNoDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::BUY, PutCall::PUT, 0, false);
    std::shared_ptr<EuropeanOption> purchasedCallDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::BUY, PutCall::CALL, premium, true);
    std::shared_ptr<EuropeanOption> purchasedCallNoDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::BUY, PutCall::CALL, 0, false);
    instruments.clear();
    instruments.push_back(purchasedPutDP);
    instruments.push_back(purchasedPutNoDP);
    instruments.push_back(purchasedCallDP);
    instruments.push_back(purchasedCallNoDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());
    double expectedMtM = structure->getValue(re);
    double expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    double expectedMtMPlusAddOn =
        expectedMtM + (spot * (2 * volume) * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      2 * EPSILON_FOR_DOUBLES * volume);

    // b) sold option
    std::shared_ptr<EuropeanOption> soldPutDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::SELL, PutCall::PUT, premium, true);
    std::shared_ptr<EuropeanOption> soldPutNoDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::SELL, PutCall::PUT, 0, false);
    std::shared_ptr<EuropeanOption> soldCallDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::SELL, PutCall::CALL, premium, true);
    std::shared_ptr<EuropeanOption> soldCallNoDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::SELL, PutCall::CALL, 0, false);
    instruments.clear();
    instruments.push_back(soldPutDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() -
                          (instruments[0]->getPremiumAmount() * instruments[0]->getVolume()),
                      EPSILON_FOR_DOUBLES);
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = cemAddOnFactor(factors, yearsToSettlement);
    expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    expectedMtMPlusAddOn = min(expectedMtMPlusAddOn, structure->getCap());
    BOOST_CHECK_SMALL(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn,
                      EPSILON_FOR_DOUBLES * volume);

    instruments.clear();
    instruments.push_back(soldPutNoDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() -
                          (instruments[0]->getPremiumAmount() * instruments[0]->getVolume()),
                      EPSILON_FOR_DOUBLES);

    instruments.clear();
    instruments.push_back(soldCallDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() -
                          (instruments[0]->getPremiumAmount() * instruments[0]->getVolume()),
                      EPSILON_FOR_DOUBLES);

    instruments.clear();
    instruments.push_back(soldCallNoDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() -
                          (instruments[0]->getPremiumAmount() * instruments[0]->getVolume()),
                      EPSILON_FOR_DOUBLES);

    // c) Put spread
    soldPutDP = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 80, volume,
                                                            BuySell::SELL, PutCall::PUT, premium, true);
    purchasedPutNoDP = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 100, volume,
                                                                   BuySell::BUY, PutCall::PUT, 0, false);
    instruments.clear();
    instruments.push_back(soldPutDP);
    instruments.push_back(purchasedPutNoDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    BOOST_CHECK(structure->isExposureCapped());
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    double expectedCap;
    expectedCap = (purchasedPutNoDP->getStrike() - purchasedPutNoDP->getPremiumAmount());
    expectedCap -= (soldPutDP->getStrike() - soldPutDP->getPremiumAmount());
    expectedCap *= soldPutDP->getVolume();
    BOOST_CHECK_SMALL(structure->getCap() - expectedCap, EPSILON_FOR_DOUBLES * volume);

    // d) Producer capped collar
    soldPutNoDP = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 80, volume,
                                                              BuySell::SELL, PutCall::PUT, 0, false);
    purchasedCallNoDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 100, volume, BuySell::BUY, PutCall::CALL, 0, false);
    soldCallNoDP = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 120, volume,
                                                               BuySell::SELL, PutCall::CALL, 0, false);
    instruments.clear();
    instruments.push_back(soldPutNoDP);
    instruments.push_back(purchasedCallNoDP);
    instruments.push_back(soldCallNoDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    expectedCap = (soldCallNoDP->getStrike() - soldCallNoDP->getPremiumAmount());
    expectedCap -= (purchasedCallNoDP->getStrike() - purchasedCallNoDP->getPremiumAmount());
    expectedCap *= soldCallNoDP->getVolume();
    BOOST_CHECK_SMALL(structure->getCap() - expectedCap, EPSILON_FOR_DOUBLES * volume);
}
BOOST_AUTO_TEST_CASE(testAsianOption)
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, BuySell::BUY);
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<CemNettingSet> structure;
    double premium = 3.4;
    double volume = 10000;
    int numberOfAveragingPoints = 20;
    AssetClass factors = getDefaultRegCapUnderlyingFactors();

    // a) purchased option
    std::shared_ptr<AsianOption> purchasedPutDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 80,
                                                 volume, BuySell::BUY, PutCall::PUT, premium, true);
    std::shared_ptr<AsianOption> purchasedPutNoDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 80,
                                                 volume, BuySell::BUY, PutCall::PUT, 0, false);
    std::shared_ptr<AsianOption> purchasedCallDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 80,
                                                 volume, BuySell::BUY, PutCall::CALL, premium, true);
    std::shared_ptr<AsianOption> purchasedCallNoDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 80,
                                                 volume, BuySell::BUY, PutCall::CALL, 0, false);
    instruments.clear();
    instruments.push_back(purchasedPutDP);
    instruments.push_back(purchasedPutNoDP);
    instruments.push_back(purchasedCallDP);
    instruments.push_back(purchasedCallNoDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());

    // b) sold option
    std::shared_ptr<AsianOption> soldPutDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 80,
                                                 volume, BuySell::SELL, PutCall::PUT, premium, true);
    std::shared_ptr<AsianOption> soldPutNoDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 80,
                                                 volume, BuySell::SELL, PutCall::PUT, 0, false);
    std::shared_ptr<AsianOption> soldCallDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 80,
                                                 volume, BuySell::SELL, PutCall::CALL, premium, true);
    std::shared_ptr<AsianOption> soldCallNoDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 80,
                                                 volume, BuySell::SELL, PutCall::CALL, 0, false);
    instruments.clear();
    instruments.push_back(soldPutDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() -
                          (instruments[0]->getPremiumAmount() * instruments[0]->getVolume()),
                      EPSILON_FOR_DOUBLES);

    instruments.clear();
    instruments.push_back(soldPutNoDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() -
                          (instruments[0]->getPremiumAmount() * instruments[0]->getVolume()),
                      EPSILON_FOR_DOUBLES);

    instruments.clear();
    instruments.push_back(soldCallDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() -
                          (instruments[0]->getPremiumAmount() * instruments[0]->getVolume()),
                      EPSILON_FOR_DOUBLES);

    instruments.clear();
    instruments.push_back(soldCallNoDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK_SMALL(structure->getCap() -
                          (instruments[0]->getPremiumAmount() * instruments[0]->getVolume()),
                      EPSILON_FOR_DOUBLES);

    // c) Put spread
    soldPutDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 80,
                                                 volume, BuySell::SELL, PutCall::PUT, premium, true);
    purchasedPutNoDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 100,
                                                 volume, BuySell::BUY, PutCall::PUT, 0, false);
    instruments.clear();
    instruments.push_back(soldPutDP);
    instruments.push_back(purchasedPutNoDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    BOOST_CHECK(structure->isExposureCapped());
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    double expectedCap;
    expectedCap = (purchasedPutNoDP->getStrike() - purchasedPutNoDP->getPremiumAmount());
    expectedCap -= (soldPutDP->getStrike() - soldPutDP->getPremiumAmount());
    expectedCap *= soldPutDP->getVolume();
    BOOST_CHECK_SMALL(structure->getCap() - expectedCap, EPSILON_FOR_DOUBLES * volume);

    // d) Producer capped collar
    soldPutNoDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 80,
                                                 volume, BuySell::SELL, PutCall::PUT, 0, false);
    purchasedCallNoDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 100,
                                                 volume, BuySell::BUY, PutCall::CALL, 0, false);
    soldCallNoDP =
        GenerateContracts::getDefaultAsianOption(anchorDate, yearsToSettlement, numberOfAveragingPoints, 120,
                                                 volume, BuySell::SELL, PutCall::CALL, 0, false);
    instruments.clear();
    instruments.push_back(soldPutNoDP);
    instruments.push_back(purchasedCallNoDP);
    instruments.push_back(soldCallNoDP);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    expectedCap = (soldCallNoDP->getStrike() - soldCallNoDP->getPremiumAmount());
    expectedCap -= (purchasedCallNoDP->getStrike() - purchasedCallNoDP->getPremiumAmount());
    expectedCap *= soldCallNoDP->getVolume();
    BOOST_CHECK_SMALL(structure->getCap() - expectedCap, EPSILON_FOR_DOUBLES * volume);
}
BOOST_AUTO_TEST_CASE(testMixedStructures)
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, BuySell::BUY);
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<CemNettingSet> structure;
    [[maybe_unused]] double premium = 3.4;
    double volume = 9087;
    AssetClass factors = getDefaultRegCapUnderlyingFactors();

    // a) Capped Producer forward
    std::shared_ptr<EuropeanForward> ef = GenerateContracts::getDefaultEuropeanForward(
        anchorDate, yearsToSettlement, 100, volume, BuySell::BUY);
    std::shared_ptr<EuropeanOption> call = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 120, volume, BuySell::SELL, PutCall::CALL, 0, false);
    instruments.clear();
    instruments.push_back(ef);
    instruments.push_back(call);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    double expectedCap = 0;
    expectedCap = (call->getStrike() - ef->getStrike());
    expectedCap *= call->getVolume();
    BOOST_CHECK_SMALL(structure->getCap() - expectedCap, EPSILON_FOR_DOUBLES * volume);
}
BOOST_AUTO_TEST_CASE(testSpecialCases)
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<CemNettingSet> structure;
    [[maybe_unused]] double premium = 0;
    double volume = 9087;
    AssetClass factors = getDefaultRegCapUnderlyingFactors();

    // a) ______/\______
    double peakStrike = 102.73;
    std::shared_ptr<EuropeanOption> c1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BuySell::BUY, PutCall::CALL, 0, false);
    std::shared_ptr<EuropeanOption> c2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, peakStrike, volume, BuySell::SELL, PutCall::CALL, 0, false);
    std::shared_ptr<EuropeanOption> c3 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, peakStrike, volume, BuySell::SELL, PutCall::CALL, 0, false);
    std::shared_ptr<EuropeanOption> c4 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 120, volume, BuySell::BUY, PutCall::CALL, 0, false);
    instruments.clear();
    instruments.push_back(c1);
    instruments.push_back(c2);
    instruments.push_back(c3);
    instruments.push_back(c4);
    structure = std::make_shared<CemNettingSet>(instruments, factors);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    double actualCap = structure->getCap();
    double expectedCap = (c2->getStrike() - c1->getStrike());
    expectedCap *= c1->getVolume();
    BOOST_CHECK_SMALL(actualCap - expectedCap, EPSILON_FOR_DOUBLES * volume);
    // Now replace the 2 solf contracts c2 and c3 with one contract with the combined volume
    std::shared_ptr<EuropeanOption> c5 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, peakStrike, 2 * volume, BuySell::SELL, PutCall::CALL, 0, false);
    instruments.clear();
    instruments.push_back(c1);
    instruments.push_back(c5);
    instruments.push_back(c4);
    Result<vector<std::shared_ptr<CemNettingSet>>> built =
        CemNettingSet::createStructures(instruments, factors);
    BOOST_REQUIRE(built.ok());
    vector<std::shared_ptr<CemNettingSet>> structures = built.value;
    BOOST_CHECK(structures.size() == 1);
    BOOST_CHECK(structures[0]->isExposureCapped());
    actualCap = structures[0]->getCap();
    BOOST_CHECK_SMALL(actualCap - expectedCap, EPSILON_FOR_DOUBLES * volume);
    // also confirm that the order of the structures is not important
    instruments.clear();
    instruments.push_back(c1);
    instruments.push_back(c4);
    instruments.push_back(c5);
    built = CemNettingSet::createStructures(instruments, factors);
    BOOST_REQUIRE(built.ok());
    structures = built.value;
    BOOST_CHECK(structures.size() == 1);
    BOOST_CHECK(structures[0]->isExposureCapped());
    actualCap = structures[0]->getCap();
    BOOST_CHECK_SMALL(actualCap - expectedCap, EPSILON_FOR_DOUBLES * volume);
}
BOOST_AUTO_TEST_CASE(testStructureCreation)
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    double structureVolume = 10000;
    double premiumRecievable = 2000;
    std::shared_ptr<EuropeanForward> fwdSale = GenerateContracts::getDefaultEuropeanForward(
        anchorDate, yearsToSettlement, 112, structureVolume, BuySell::SELL);
    std::shared_ptr<SingleInstrumentContract> cf = GenerateContracts::getDefaultFixedCashflowWithPremium(
        anchorDate, yearsToSettlement, premiumRecievable, BuySell::BUY);
    AssetClass regCapFactors = getDefaultRegCapUnderlyingFactors();
    std::shared_ptr<EuropeanOption> option = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 115, structureVolume, BuySell::BUY, PutCall::CALL, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> contracts;
    contracts.push_back(fwdSale);
    contracts.push_back(cf);
    contracts.push_back(option);

    Result<vector<std::shared_ptr<CemNettingSet>>> built =
        CemNettingSet::createStructures(contracts, regCapFactors);
    BOOST_CHECK(built.ok());
    vector<std::shared_ptr<CemNettingSet>> structures = built.value;
    BOOST_REQUIRE(structures.size() == 1);
    BOOST_CHECK(structures[0]->isOK());

    option = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 115, structureVolume * 1.05, BuySell::BUY, PutCall::CALL, 0);
    contracts.clear();
    contracts.push_back(fwdSale);
    contracts.push_back(cf);
    contracts.push_back(option);
    built = CemNettingSet::createStructures(contracts, regCapFactors);
    BOOST_CHECK(built.ok());
    structures = built.value;
    BOOST_REQUIRE(structures.size() == 1);
    BOOST_CHECK(structures[0]->isOK());

    // Test a call spread manually and automatically
    std::shared_ptr<EuropeanOption> put = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, structureVolume, BuySell::SELL, PutCall::PUT, 0);
    std::shared_ptr<EuropeanOption> purchasedCall = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 115, structureVolume, BuySell::BUY, PutCall::CALL, 0);
    std::shared_ptr<EuropeanOption> soldCall = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 130, structureVolume, BuySell::SELL, PutCall::CALL, 0);
    vector<std::shared_ptr<SingleInstrumentContract>> threeway;
    threeway.push_back(put);
    threeway.push_back(purchasedCall);
    threeway.push_back(soldCall);
    CemNettingSet manualStructure(threeway, regCapFactors);
    structures = CemNettingSet::createStructures(threeway, regCapFactors).value;
    BOOST_REQUIRE(structures.size() == 1);
    std::shared_ptr<CemNettingSet> autoStructure = structures[0];
    BOOST_CHECK(autoStructure->isOK());
    BOOST_CHECK_SMALL(autoStructure->getMtMPlusAddOn(re) - manualStructure.getMtMPlusAddOn(re),
                      EPSILON_FOR_DOUBLES * structureVolume);

    // Test most complex case
    std::shared_ptr<EuropeanOption> s1p1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, structureVolume, BuySell::BUY, PutCall::PUT, 0);
    std::shared_ptr<EuropeanOption> s1p2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 85, 2 * structureVolume, BuySell::BUY, PutCall::PUT, 0);
    std::shared_ptr<EuropeanOption> s1p3 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 85, structureVolume, BuySell::BUY, PutCall::PUT, 0);

    std::shared_ptr<EuropeanOption> s1c1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 100, 6 * structureVolume, BuySell::BUY, PutCall::CALL, 0);
    std::shared_ptr<EuropeanOption> s1c2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 110, 6 * structureVolume, BuySell::SELL, PutCall::CALL, 0);
    std::shared_ptr<EuropeanOption> s1c3 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 110, 6 * structureVolume, BuySell::SELL, PutCall::CALL, 0);
    std::shared_ptr<EuropeanOption> s1c4 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 120, 6 * structureVolume, BuySell::BUY, PutCall::CALL, 0);

    std::shared_ptr<EuropeanOption> s1c5 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 130, 2 * structureVolume, BuySell::BUY, PutCall::CALL, 0);
    std::shared_ptr<EuropeanOption> s1c6 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 130, structureVolume, BuySell::BUY, PutCall::CALL, 0);
    std::shared_ptr<EuropeanOption> s1c7 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 140, structureVolume, BuySell::SELL, PutCall::CALL, 0);

    contracts.clear();
    contracts.push_back(s1p1);
    contracts.push_back(s1p2);
    contracts.push_back(s1p3);
    contracts.push_back(s1c1);
    contracts.push_back(s1c2);
    contracts.push_back(s1c3);
    contracts.push_back(s1c4);
    contracts.push_back(s1c5);
    contracts.push_back(s1c6);
    contracts.push_back(s1c7);

    Result<vector<std::shared_ptr<CemNettingSet>>> split =
        CemNettingSet::createStructures(contracts, regCapFactors);
    BOOST_REQUIRE(split.ok());
    vector<std::shared_ptr<CemNettingSet>> s1 = split.value;
    BOOST_REQUIRE(s1.size() == 2);
    BOOST_CHECK(s1[0]->isOK());
    BOOST_CHECK(s1[1]->isOK());
    BOOST_CHECK(s1[0]->isExposureCapped());
    BOOST_CHECK(!s1[1]->isExposureCapped());

    vector<double> testCases;
    testCases.push_back(70);
    testCases.push_back(80);
    testCases.push_back(82.5);
    testCases.push_back(85);
    testCases.push_back(95);
    testCases.push_back(100);
    testCases.push_back(105);
    testCases.push_back(110);
    testCases.push_back(115);
    testCases.push_back(120);
    testCases.push_back(125);
    testCases.push_back(130);
    testCases.push_back(135);
    testCases.push_back(140);
    testCases.push_back(145);
    for (size_t i = 0; i < testCases.size(); ++i)
    {
        double structrePayoff = s1[0]->getPayoff(testCases[i]);
        structrePayoff += s1[1]->getPayoff(testCases[i]);
        double underlyingPayoff = 0;
        for (size_t j = 0; j < contracts.size(); ++j)
        {
            underlyingPayoff += contracts[j]->getPayoff(testCases[i]);
        }
        BOOST_CHECK_SMALL(structrePayoff - underlyingPayoff, 1e-8);
    }
    double v1 = s1[0]->getValue(re);
    double v2 = s1[1]->getValue(re);
    double c = 0;
    for (size_t j = 0; j < contracts.size(); ++j)
    {
        c += contracts[j]->getValue(re);
    }
    BOOST_CHECK_SMALL(v1 + v2 - c, 1e-8);
}

BOOST_AUTO_TEST_SUITE_END()
