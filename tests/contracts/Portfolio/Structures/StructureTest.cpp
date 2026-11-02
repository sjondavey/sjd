#include "StructureTest.h"

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace boost::math;
using namespace sjd;

std::shared_ptr<RegCapUnderlyingFactors> StructureTest::getDefaultRegCapUnderlyingFactors()
{
    return std::shared_ptr<RegCapUnderlyingFactors>(new CommodityRegCapFactors);
}

void StructureTest::testGradient()
{
    BOOST_TEST_MESSAGE("Testing Structures ...");

    Date anchorDate = Date(27, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 12300;

    std::shared_ptr<SingleInstrumentContract> call1 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 80, volume, BUY, CALL, 0);
    std::shared_ptr<SingleInstrumentContract> call2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 120, 2 * volume, SELL, CALL, 0);
    std::shared_ptr<SingleInstrumentContract> call3 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 160, volume, BUY, CALL, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    instruments.push_back(call1);
    instruments.push_back(call2);
    instruments.push_back(call3);

    double g = Structure::gradient(instruments, 60, 70);
    BOOST_CHECK(abs(g) < EPSILON_FOR_DOUBLES);

    g = Structure::gradient(instruments, 80, 85);
    BOOST_CHECK(abs(g - volume) < EPSILON_FOR_DOUBLES);

    g = Structure::gradient(instruments, 120, 130);
    BOOST_CHECK(abs(g + volume) < EPSILON_FOR_DOUBLES);

    g = Structure::gradient(instruments, 160, 170);
    BOOST_CHECK(abs(g) < EPSILON_FOR_DOUBLES);
}

void StructureTest::testIsUncappedOnTheLeft()
{
    Date anchorDate = Date(27, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 12300;

    std::shared_ptr<SingleInstrumentContract> put1 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 80, volume, BUY, PUT, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    instruments.push_back(put1);

    double effectiveGradient;
    bool uncapped = Structure::isUncappedOnTheLeft(instruments, effectiveGradient);
    BOOST_CHECK(uncapped);
    BOOST_CHECK(abs(effectiveGradient - (-volume)) < volume * EPSILON_FOR_DOUBLES);
    double g = Structure::gradient(instruments, 40, 50);
    BOOST_CHECK(abs(effectiveGradient - g) < volume * EPSILON_FOR_DOUBLES);

    std::shared_ptr<SingleInstrumentContract> put2 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 70, volume, SELL, PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    uncapped = Structure::isUncappedOnTheLeft(instruments, effectiveGradient);
    BOOST_CHECK(!uncapped);
    BOOST_CHECK(abs(effectiveGradient) < volume * EPSILON_FOR_DOUBLES);
    g = Structure::gradient(instruments, 40, 50);
    BOOST_CHECK(abs(effectiveGradient - g) < volume * EPSILON_FOR_DOUBLES);

    put2 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 70, volume, BUY, PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    uncapped = Structure::isUncappedOnTheLeft(instruments, effectiveGradient);
    BOOST_CHECK(uncapped);
    BOOST_CHECK(abs(effectiveGradient - (-2 * volume)) < volume * EPSILON_FOR_DOUBLES);
    g = Structure::gradient(instruments, 40, 50);
    BOOST_CHECK(abs(effectiveGradient - g) < volume * EPSILON_FOR_DOUBLES);

    std::shared_ptr<SingleInstrumentContract> put3 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 60, volume, SELL, PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    instruments.push_back(put3);
    uncapped = Structure::isUncappedOnTheLeft(instruments, effectiveGradient);
    BOOST_CHECK(uncapped);
    BOOST_CHECK(abs(effectiveGradient - (-volume)) < volume * EPSILON_FOR_DOUBLES);
    g = Structure::gradient(instruments, 40, 50);
    BOOST_CHECK(abs(effectiveGradient - g) < volume * EPSILON_FOR_DOUBLES);
}

void StructureTest::testGetContractsWithUncappedExposureOnTheLeft()
{
    Date anchorDate = Date(27, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 12300;

    std::shared_ptr<SingleInstrumentContract> put1 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 80, volume, BUY, PUT, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    instruments.push_back(put1);

    double commonStrike;
    vector<std::shared_ptr<SingleInstrumentContract>> uncappedContracts;
    uncappedContracts = Structure::getContractsWithUncappedExposureOnTheLeft(instruments, commonStrike);
    BOOST_CHECK(uncappedContracts.size() == 1);
    BOOST_CHECK(uncappedContracts[0] == put1);

    std::shared_ptr<SingleInstrumentContract> put2 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 70, volume, SELL, PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    uncappedContracts = Structure::getContractsWithUncappedExposureOnTheLeft(instruments, commonStrike);
    BOOST_CHECK(uncappedContracts.size() == 1);
    BOOST_CHECK(uncappedContracts[0] == put1);

    put2 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 70, volume, BUY, PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    uncappedContracts = Structure::getContractsWithUncappedExposureOnTheLeft(instruments, commonStrike);
    BOOST_CHECK(uncappedContracts.size() == 1);
    BOOST_CHECK(uncappedContracts[0] == put2);

    put2 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 80, volume, BUY, PUT, 0);
    instruments.clear();
    instruments.push_back(put1);
    instruments.push_back(put2);
    uncappedContracts = Structure::getContractsWithUncappedExposureOnTheLeft(instruments, commonStrike);
    BOOST_CHECK(uncappedContracts.size() == 2);
    BOOST_CHECK(uncappedContracts[0] == put1);
    BOOST_CHECK(uncappedContracts[1] == put2);
}

void StructureTest::testQualifyForRegulatoryCapitalOffset()
{
    Date anchorDate = Date(27, Jan, 2013);
    int yearsToSettlement = 2;
    std::shared_ptr<SingleInstrumentContract> fwd =
        GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToSettlement, 112, 45, BUY);
    std::shared_ptr<SingleInstrumentContract> option =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 115, 45, SELL, CALL, 0);
    std::shared_ptr<SingleInstrumentContract> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, SELL);

    string errorMsg;
    BOOST_CHECK(Structure::qualifyForRegulatoryCapitalOffset(fwd, cf, errorMsg));
    BOOST_CHECK(Structure::qualifyForRegulatoryCapitalOffset(fwd, option, errorMsg));

    option = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement + 1, 115, 45, SELL,
                                                         CALL, 0);
    BOOST_CHECK(!Structure::qualifyForRegulatoryCapitalOffset(fwd, option, errorMsg));

    int numberOfAveragingDates = 30;
    double volume = 47;
    std::shared_ptr<SingleDerivativeContract> asianOption = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingDates, 90, volume, BUY, CALL, 10, true);
    BOOST_CHECK(Structure::qualifyForRegulatoryCapitalOffset(cf, asianOption, errorMsg));
    BOOST_CHECK(!Structure::qualifyForRegulatoryCapitalOffset(fwd, asianOption, errorMsg));

    std::shared_ptr<SingleDerivativeContract> asianFwd = GenerateContracts::getDefaultAsianForward(
        anchorDate, yearsToSettlement, numberOfAveragingDates, 90, volume, BUY);
    BOOST_CHECK(Structure::qualifyForRegulatoryCapitalOffset(asianFwd, asianOption, errorMsg));

    std::shared_ptr<SingleDerivativeContract> asianFwd2 = GenerateContracts::getDefaultAsianForward(
        anchorDate, yearsToSettlement, numberOfAveragingDates - 1, 90, volume, BUY);
    BOOST_CHECK(!Structure::qualifyForRegulatoryCapitalOffset(asianFwd, asianFwd2, errorMsg));
}

void StructureTest::testMoveContracts()
{
    Date anchorDate = Date(27, Jan, 2013);
    int yearsToSettlement = 2;
    double volume = 12300;

    std::shared_ptr<SingleInstrumentContract> put1 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 80, volume, BUY, PUT, 0);
    std::shared_ptr<SingleInstrumentContract> put2 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 70, volume, BUY, PUT, 0);
    std::shared_ptr<SingleInstrumentContract> call1 =
        GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 120, volume, BUY, CALL, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> inputInstruments, identifiedInstruments,
        outputInstruments;
    inputInstruments.push_back(put1);
    inputInstruments.push_back(put2);
    inputInstruments.push_back(call1);

    identifiedInstruments.push_back(call1);

    bool sucess = Structure::moveContracts(identifiedInstruments, inputInstruments, outputInstruments);
    BOOST_REQUIRE(sucess);
    BOOST_CHECK(inputInstruments.size() == 2);
    BOOST_CHECK(outputInstruments.size() == 1);
    BOOST_CHECK(outputInstruments[0] == call1);

    sucess = Structure::moveContracts(identifiedInstruments, inputInstruments, outputInstruments);
    BOOST_REQUIRE(!sucess);

    identifiedInstruments.clear();
    identifiedInstruments.push_back(put1);
    sucess = Structure::moveContracts(identifiedInstruments, inputInstruments, outputInstruments);
    BOOST_REQUIRE(sucess);
    BOOST_CHECK(inputInstruments.size() == 1);
    BOOST_CHECK(outputInstruments.size() == 2);
    BOOST_CHECK(outputInstruments[1] == put1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void StructureTest::testSettlementDate()
{

    Date anchorDate = Date(27, Jan, 2013);
    int yearsToMaturity = 2;
    std::shared_ptr<ForwardContract> fwd =
        GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToMaturity, 112, 45, BUY);
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 5, SELL);
    std::shared_ptr<RegCapUnderlyingFactors> factors = getDefaultRegCapUnderlyingFactors();

    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    instruments.push_back(fwd);
    instruments.push_back(cf);

    Structure structure(instruments, factors);

    Date sd1 = fwd->getSettlementDate();
    Date sd2 = structure.getSettlementDate();

    BOOST_REQUIRE(structure.isOK());
    BOOST_CHECK(sd1 == sd2);
}

void StructureTest::testCashFlowN()
{

    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForCashflow(anchorDate, vector<Date>(0));

    int yearsToMaturity = 2;
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 5, BUY);
    std::shared_ptr<RegCapUnderlyingFactors> factors = getDefaultRegCapUnderlyingFactors();

    // std::shared_ptr<DayCounter> dayCounter = std::shared_ptr<DayCounter>(new Actual365Fixed());
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;
    // std::shared_ptr<DiscountCurve> dc = gr.getDiscountCurve(dayCounter, nullCalendar, anchorDate, 5, 0.05);
    double ngr = .36;
    set<Date> forwardDates;
    forwardDates.insert(nullCalendar->advance(anchorDate, 1, Years));
    forwardDates.insert(nullCalendar->advance(anchorDate, 2, Years));
    vector<double> forwardSpots1 = vector<double>(2, 80.0);
    vector<double> forwardSpots2 = vector<double>(2, 100.0);
    vector<double> forwardNgrs = vector<double>(2, ngr);

    // a) no premium
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    instruments.push_back(cf);
    std::shared_ptr<Structure> structure = std::shared_ptr<Structure>(new Structure(instruments, factors));

    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_REQUIRE(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - cf->getVolume()) < EPSILON_FOR_DOUBLES);
    double expectedMtM = cf->getValue(re);
    BOOST_CHECK(abs(structure->getValue(re) - expectedMtM) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - cf->getVolume()) < EPSILON_FOR_DOUBLES);

    // b) premium
    cf = GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 5, BUY);
    instruments.clear();
    instruments.push_back(cf);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_REQUIRE(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (cf->getVolume() - cf->getPremiumAmount())) < EPSILON_FOR_DOUBLES);
    expectedMtM = cf->getValue(re);
    BOOST_CHECK(abs(structure->getValue(re) - expectedMtM) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - (cf->getVolume() - cf->getPremiumAmount())) <
                EPSILON_FOR_DOUBLES);

    // c) sold cashflow
    cf = GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 5, SELL);
    instruments.clear();
    instruments.push_back(cf);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_REQUIRE(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() + ((cf->getVolume() - cf->getPremiumAmount()))) <
                EPSILON_FOR_DOUBLES);
    expectedMtM = cf->getValue(re);
    BOOST_CHECK(abs(structure->getValue(re) - expectedMtM) < EPSILON_FOR_DOUBLES);
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) + (cf->getVolume() - cf->getPremiumAmount())) <
                EPSILON_FOR_DOUBLES);

    // d multiple cash flows
    cf = GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 5, BUY);
    std::shared_ptr<FixedCashflow> cf2 =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 2, SELL);
    std::shared_ptr<FixedCashflow> cf3 =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToMaturity, 1, BUY);
    instruments.clear();
    instruments.push_back(cf);
    instruments.push_back(cf2);
    instruments.push_back(cf3);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_REQUIRE(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (5 - 2 + 1)) < EPSILON_FOR_DOUBLES);
}

void StructureTest::testEuropeanForward()
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re =
        gr.getRatesEnvironmentForEuropeanForward(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;
    Date sd = nullCalendar->advance(anchorDate, 2, Years);
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, BUY);
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<Structure> structure;
    std::shared_ptr<RegCapUnderlyingFactors> factors = getDefaultRegCapUnderlyingFactors();
    double volume = 10000;
    double spot = re->getFRS()->getForward(re->getAnchorDate());
    double ngr = .36;

    // a) purchased forward
    std::shared_ptr<EuropeanForward> ef =
        GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToSettlement, 100, volume, BUY);
    instruments.clear();
    instruments.push_back(ef);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());
    double expectedMtM = structure->getValue(re);
    double expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    double expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                EPSILON_FOR_DOUBLES * volume);

    // b) Forward with premium
    instruments.clear();
    instruments.push_back(ef);
    instruments.push_back(cf);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());

    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                EPSILON_FOR_DOUBLES * volume);

    // c) Buy and sell forward - lock in profit
    std::shared_ptr<EuropeanForward> ef2 =
        GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToSettlement, 120, volume, SELL);
    instruments.clear();
    instruments.push_back(ef);
    instruments.push_back(ef2);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (120 - 100) * volume) < EPSILON_FOR_DOUBLES);

    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    expectedMtMPlusAddOn =
        (120 - 100) * volume; // + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                EPSILON_FOR_DOUBLES * volume);

    // d) Buy and sell forward - lock in loss
    ef2 = GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToSettlement, 80, volume, SELL);
    instruments.clear();
    instruments.push_back(ef);
    instruments.push_back(ef2);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(!structure->getHasPriceExposure());
    BOOST_CHECK(abs(structure->getCap() - (80 - 100) * volume) < EPSILON_FOR_DOUBLES * volume);
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    expectedMtMPlusAddOn = (80 - 100) * volume;
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                EPSILON_FOR_DOUBLES * volume);

    // e) Portfolio of 3 forwards
    std::shared_ptr<EuropeanForward> ef3 =
        GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToSettlement, 120, volume, SELL);
    instruments.clear();
    instruments.push_back(ef);
    instruments.push_back(ef2);
    instruments.push_back(ef3);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());

    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                2 * EPSILON_FOR_DOUBLES * volume);
}

void StructureTest::testAsianforward()
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianForward(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;
    Date sd = nullCalendar->advance(anchorDate, yearsToSettlement, Years);
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, BUY);
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<Structure> structure;
    double volume = 10000;
    std::shared_ptr<RegCapUnderlyingFactors> factors = getDefaultRegCapUnderlyingFactors();

    double spot = re->getFRS()->getForward(anchorDate);
    double ngr = .23;
    set<Date> forwardDates;
    forwardDates.insert(nullCalendar->advance(anchorDate, 1, Years));
    forwardDates.insert(sd);
    vector<double> forwardSpots1 = vector<double>(2, 80.0);
    vector<double> forwardSpots2 = vector<double>(2, 100.0);
    vector<double> forwardNgrs = vector<double>(2, ngr);

    // a) purchased forward
    int numberOfAveragingPoints = 30;
    std::shared_ptr<AsianForward> af = GenerateContracts::getDefaultAsianForward(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 100, volume, BUY);
    instruments.clear();
    instruments.push_back(af);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());

    double expectedMtM = structure->getValue(re);
    double expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    double expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                EPSILON_FOR_DOUBLES * volume);

    // b) Forward with premium
    instruments.clear();
    instruments.push_back(af);
    instruments.push_back(cf);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                EPSILON_FOR_DOUBLES * volume);

    // c) Buy and sell forward - lock in profit
    std::shared_ptr<AsianForward> af2 = GenerateContracts::getDefaultAsianForward(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 120, volume, SELL);
    instruments.clear();
    instruments.push_back(af);
    instruments.push_back(af2);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(!structure->getHasPriceExposure());
    BOOST_CHECK(abs(structure->getCap() - (120 - 100) * volume) < EPSILON_FOR_DOUBLES);
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    expectedMtMPlusAddOn =
        (120 - 100) * volume; // + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                EPSILON_FOR_DOUBLES * volume);

    // d) Buy and sell forward - lock in loss
    af2 = GenerateContracts::getDefaultAsianForward(anchorDate, yearsToSettlement, numberOfAveragingPoints,
                                                    80, volume, SELL);
    instruments.clear();
    instruments.push_back(af);
    instruments.push_back(af2);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(!structure->getHasPriceExposure());
    BOOST_CHECK(abs(structure->getCap() - (80 - 100) * volume) < EPSILON_FOR_DOUBLES * volume);
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    expectedMtMPlusAddOn = (80 - 100) * volume;
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                EPSILON_FOR_DOUBLES * volume);

    // e) Portfolio of 3 forwards
    std::shared_ptr<AsianForward> af3 = GenerateContracts::getDefaultAsianForward(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 120, volume, SELL);
    instruments.clear();
    instruments.push_back(af);
    instruments.push_back(af2);
    instruments.push_back(af3);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                2 * EPSILON_FOR_DOUBLES * volume);
}

void StructureTest::testEuropeanOption()
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re =
        gr.getRatesEnvironmentForEuropeanOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    std::shared_ptr<Calendar> nullCalendar = gr.defaultCalendar;
    Date sd = nullCalendar->advance(anchorDate, yearsToSettlement, Years);
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, BUY);
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<Structure> structure;
    double premium = 3.4;
    double volume = 10000;
    std::shared_ptr<RegCapUnderlyingFactors> factors = getDefaultRegCapUnderlyingFactors();
    std::shared_ptr<DiscountCurve> dc;
    std::shared_ptr<ForwardCurveDeterministic> fc;
    std::shared_ptr<VolatilityCurve> vc;
    double spot = re->getFRS()->getForward(anchorDate);
    double ngr = .23;
    set<Date> forwardDates;
    forwardDates.insert(nullCalendar->advance(anchorDate, 1, Years));
    forwardDates.insert(sd);
    vector<double> forwardSpots1 = vector<double>(2, 80.0);
    vector<double> forwardSpots2 = vector<double>(2, 100.0);
    vector<double> forwardNgrs = vector<double>(2, ngr);

    // a) purchased option
    std::shared_ptr<EuropeanOption> purchasedPutDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BUY, PUT, premium, true);
    std::shared_ptr<EuropeanOption> purchasedPutNoDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BUY, PUT, 0, false);
    std::shared_ptr<EuropeanOption> purchasedCallDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BUY, CALL, premium, true);
    std::shared_ptr<EuropeanOption> purchasedCallNoDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BUY, CALL, 0, false);
    instruments.clear();
    instruments.push_back(purchasedPutDP);
    instruments.push_back(purchasedPutNoDP);
    instruments.push_back(purchasedCallDP);
    instruments.push_back(purchasedCallNoDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());
    double expectedMtM = structure->getValue(re);
    double expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    double expectedMtMPlusAddOn =
        expectedMtM + (spot * (2 * volume) * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                2 * EPSILON_FOR_DOUBLES * volume);

    // b) sold option
    std::shared_ptr<EuropeanOption> soldPutDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, SELL, PUT, premium, true);
    std::shared_ptr<EuropeanOption> soldPutNoDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, SELL, PUT, 0, false);
    std::shared_ptr<EuropeanOption> soldCallDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, SELL, CALL, premium, true);
    std::shared_ptr<EuropeanOption> soldCallNoDP = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, SELL, CALL, 0, false);
    instruments.clear();
    instruments.push_back(soldPutDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (instruments[0]->getPremiumAmount() *
                                           instruments[0]->getVolume())) < EPSILON_FOR_DOUBLES);
    expectedMtM = structure->getValue(re);
    expectedCommodityFactor = factors->getFactor(yearsToSettlement);
    expectedMtMPlusAddOn = expectedMtM + (spot * volume * expectedCommodityFactor * (0.4 + 0.6 * ngr));
    expectedMtMPlusAddOn = min(expectedMtMPlusAddOn, structure->getCap());
    BOOST_CHECK(abs(structure->getMtMPlusAddOn(re, ngr) - expectedMtMPlusAddOn) <
                EPSILON_FOR_DOUBLES * volume);

    instruments.clear();
    instruments.push_back(soldPutNoDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (instruments[0]->getPremiumAmount() *
                                           instruments[0]->getVolume())) < EPSILON_FOR_DOUBLES);

    instruments.clear();
    instruments.push_back(soldCallDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (instruments[0]->getPremiumAmount() *
                                           instruments[0]->getVolume())) < EPSILON_FOR_DOUBLES);

    instruments.clear();
    instruments.push_back(soldCallNoDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (instruments[0]->getPremiumAmount() *
                                           instruments[0]->getVolume())) < EPSILON_FOR_DOUBLES);

    // c) Put spread
    soldPutDP = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 80, volume, SELL,
                                                            PUT, premium, true);
    purchasedPutNoDP = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 100, volume,
                                                                   BUY, PUT, 0, false);
    instruments.clear();
    instruments.push_back(soldPutDP);
    instruments.push_back(purchasedPutNoDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    BOOST_CHECK(structure->isExposureCapped());
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    double expectedCap;
    expectedCap = (purchasedPutNoDP->getStrike() - purchasedPutNoDP->getPremiumAmount());
    expectedCap -= (soldPutDP->getStrike() - soldPutDP->getPremiumAmount());
    expectedCap *= soldPutDP->getVolume();
    BOOST_CHECK(abs(structure->getCap() - expectedCap) < EPSILON_FOR_DOUBLES * volume);

    // d) Producer capped collar
    soldPutNoDP = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 80, volume, SELL,
                                                              PUT, 0, false);
    purchasedCallNoDP = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 100,
                                                                    volume, BUY, CALL, 0, false);
    soldCallNoDP = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 120, volume,
                                                               SELL, CALL, 0, false);
    instruments.clear();
    instruments.push_back(soldPutNoDP);
    instruments.push_back(purchasedCallNoDP);
    instruments.push_back(soldCallNoDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    expectedCap = (soldCallNoDP->getStrike() - soldCallNoDP->getPremiumAmount());
    expectedCap -= (purchasedCallNoDP->getStrike() - purchasedCallNoDP->getPremiumAmount());
    expectedCap *= soldCallNoDP->getVolume();
    BOOST_CHECK(abs(structure->getCap() - expectedCap) < EPSILON_FOR_DOUBLES * volume);
}

void StructureTest::testAsianOption()
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, BUY);
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<Structure> structure;
    double premium = 3.4;
    double volume = 10000;
    int numberOfAveragingPoints = 20;
    std::shared_ptr<RegCapUnderlyingFactors> factors = getDefaultRegCapUnderlyingFactors();

    // a) purchased option
    std::shared_ptr<AsianOption> purchasedPutDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 80, volume, BUY, PUT, premium, true);
    std::shared_ptr<AsianOption> purchasedPutNoDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 80, volume, BUY, PUT, 0, false);
    std::shared_ptr<AsianOption> purchasedCallDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 80, volume, BUY, CALL, premium, true);
    std::shared_ptr<AsianOption> purchasedCallNoDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 80, volume, BUY, CALL, 0, false);
    instruments.clear();
    instruments.push_back(purchasedPutDP);
    instruments.push_back(purchasedPutNoDP);
    instruments.push_back(purchasedCallDP);
    instruments.push_back(purchasedCallNoDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(!structure->isExposureCapped());

    // b) sold option
    std::shared_ptr<AsianOption> soldPutDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 80, volume, SELL, PUT, premium, true);
    std::shared_ptr<AsianOption> soldPutNoDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 80, volume, SELL, PUT, 0, false);
    std::shared_ptr<AsianOption> soldCallDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 80, volume, SELL, CALL, premium, true);
    std::shared_ptr<AsianOption> soldCallNoDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 80, volume, SELL, CALL, 0, false);
    instruments.clear();
    instruments.push_back(soldPutDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (instruments[0]->getPremiumAmount() *
                                           instruments[0]->getVolume())) < EPSILON_FOR_DOUBLES);

    instruments.clear();
    instruments.push_back(soldPutNoDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (instruments[0]->getPremiumAmount() *
                                           instruments[0]->getVolume())) < EPSILON_FOR_DOUBLES);

    instruments.clear();
    instruments.push_back(soldCallDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (instruments[0]->getPremiumAmount() *
                                           instruments[0]->getVolume())) < EPSILON_FOR_DOUBLES);

    instruments.clear();
    instruments.push_back(soldCallNoDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    BOOST_CHECK(abs(structure->getCap() - (instruments[0]->getPremiumAmount() *
                                           instruments[0]->getVolume())) < EPSILON_FOR_DOUBLES);

    // c) Put spread
    soldPutDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 80, volume, SELL, PUT, premium, true);
    purchasedPutNoDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 100, volume, BUY, PUT, 0, false);
    instruments.clear();
    instruments.push_back(soldPutDP);
    instruments.push_back(purchasedPutNoDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    BOOST_CHECK(structure->isExposureCapped());
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    double expectedCap;
    expectedCap = (purchasedPutNoDP->getStrike() - purchasedPutNoDP->getPremiumAmount());
    expectedCap -= (soldPutDP->getStrike() - soldPutDP->getPremiumAmount());
    expectedCap *= soldPutDP->getVolume();
    BOOST_CHECK(abs(structure->getCap() - expectedCap) < EPSILON_FOR_DOUBLES * volume);

    // d) Producer capped collar
    soldPutNoDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 80, volume, SELL, PUT, 0, false);
    purchasedCallNoDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 100, volume, BUY, CALL, 0, false);
    soldCallNoDP = GenerateContracts::getDefaultAsianOption(
        anchorDate, yearsToSettlement, numberOfAveragingPoints, 120, volume, SELL, CALL, 0, false);
    instruments.clear();
    instruments.push_back(soldPutNoDP);
    instruments.push_back(purchasedCallNoDP);
    instruments.push_back(soldCallNoDP);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    expectedCap = (soldCallNoDP->getStrike() - soldCallNoDP->getPremiumAmount());
    expectedCap -= (purchasedCallNoDP->getStrike() - purchasedCallNoDP->getPremiumAmount());
    expectedCap *= soldCallNoDP->getVolume();
    BOOST_CHECK(abs(structure->getCap() - expectedCap) < EPSILON_FOR_DOUBLES * volume);
}

void StructureTest::testMixedStructures()
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    std::shared_ptr<FixedCashflow> cf =
        GenerateContracts::getDefaultFixedCashflowWithPremium(anchorDate, yearsToSettlement, 5, BUY);
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<Structure> structure;
    double premium = 3.4;
    double volume = 9087;
    std::shared_ptr<RegCapUnderlyingFactors> factors = getDefaultRegCapUnderlyingFactors();

    // a) Capped Producer forward
    std::shared_ptr<EuropeanForward> ef =
        GenerateContracts::getDefaultEuropeanForward(anchorDate, yearsToSettlement, 100, volume, BUY);
    std::shared_ptr<EuropeanOption> call = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 120, volume, SELL, CALL, 0, false);
    instruments.clear();
    instruments.push_back(ef);
    instruments.push_back(call);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    double expectedCap = 0;
    expectedCap = (call->getStrike() - ef->getStrike());
    expectedCap *= call->getVolume();
    BOOST_CHECK(abs(structure->getCap() - expectedCap) < EPSILON_FOR_DOUBLES * volume);
}

void StructureTest::testSpecialCases()
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    vector<std::shared_ptr<SingleInstrumentContract>> instruments;
    std::shared_ptr<Structure> structure;
    double premium = 0;
    double volume = 9087;
    std::shared_ptr<RegCapUnderlyingFactors> factors = getDefaultRegCapUnderlyingFactors();

    // a) ______/\______
    double peakStrike = 102.73;
    std::shared_ptr<EuropeanOption> c1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, volume, BUY, CALL, 0, false);
    std::shared_ptr<EuropeanOption> c2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, peakStrike, volume, SELL, CALL, 0, false);
    std::shared_ptr<EuropeanOption> c3 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, peakStrike, volume, SELL, CALL, 0, false);
    std::shared_ptr<EuropeanOption> c4 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 120, volume, BUY, CALL, 0, false);
    instruments.clear();
    instruments.push_back(c1);
    instruments.push_back(c2);
    instruments.push_back(c3);
    instruments.push_back(c4);
    structure = std::shared_ptr<Structure>(new Structure(instruments, factors));
    structure->validate(re);
    BOOST_REQUIRE(structure->isOK());
    BOOST_CHECK(structure->isExposureCapped());
    double actualCap = structure->getCap();
    double expectedCap = (c2->getStrike() - c1->getStrike());
    expectedCap *= c1->getVolume();
    BOOST_CHECK(abs(actualCap - expectedCap) < EPSILON_FOR_DOUBLES * volume);
    // Now replace the 2 solf contracts c2 and c3 with one contract with the combined volume
    std::shared_ptr<EuropeanOption> c5 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, peakStrike, 2 * volume, SELL, CALL, 0, false);
    instruments.clear();
    instruments.push_back(c1);
    instruments.push_back(c5);
    instruments.push_back(c4);
    bool successful = false;
    vector<std::shared_ptr<Structure>> structures =
        Structure::createStructures(instruments, factors, successful);
    BOOST_REQUIRE(successful);
    BOOST_CHECK(structures.size() == 1);
    BOOST_CHECK(structures[0]->isExposureCapped());
    actualCap = structures[0]->getCap();
    BOOST_CHECK(abs(actualCap - expectedCap) < EPSILON_FOR_DOUBLES * volume);
    // also confirm that the order of the structures is not important
    instruments.clear();
    instruments.push_back(c1);
    instruments.push_back(c4);
    instruments.push_back(c5);
    successful = false;
    structures = Structure::createStructures(instruments, factors, successful);
    BOOST_REQUIRE(successful);
    BOOST_CHECK(structures.size() == 1);
    BOOST_CHECK(structures[0]->isExposureCapped());
    actualCap = structures[0]->getCap();
    BOOST_CHECK(abs(actualCap - expectedCap) < EPSILON_FOR_DOUBLES * volume);
}

void StructureTest::testStructureCreation()
{
    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, vector<Date>(0));

    int yearsToSettlement = 2;
    double structureVolume = 10000;
    double premiumRecievable = 2000;
    std::shared_ptr<EuropeanForward> fwdSale = GenerateContracts::getDefaultEuropeanForward(
        anchorDate, yearsToSettlement, 112, structureVolume, SELL);
    std::shared_ptr<SingleInstrumentContract> cf = GenerateContracts::getDefaultFixedCashflowWithPremium(
        anchorDate, yearsToSettlement, premiumRecievable, BUY);
    std::shared_ptr<RegCapUnderlyingFactors> regCapFactors =
        StructureTest::getDefaultRegCapUnderlyingFactors();
    std::shared_ptr<EuropeanOption> option = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 115, structureVolume, BUY, CALL, 0);

    vector<std::shared_ptr<SingleInstrumentContract>> contracts;
    contracts.push_back(fwdSale);
    contracts.push_back(cf);
    contracts.push_back(option);

    vector<std::shared_ptr<Structure>> structures;
    bool successful;
    structures = Structure::createStructures(contracts, regCapFactors, successful);
    BOOST_CHECK(successful);
    BOOST_REQUIRE(structures.size() == 1);
    BOOST_CHECK(structures[0]->isOK());

    option = GenerateContracts::getDefaultEuropeanOption(anchorDate, yearsToSettlement, 115,
                                                         structureVolume * 1.05, BUY, CALL, 0);
    contracts.clear();
    contracts.push_back(fwdSale);
    contracts.push_back(cf);
    contracts.push_back(option);
    structures = Structure::createStructures(contracts, regCapFactors, successful);
    BOOST_CHECK(successful);
    BOOST_REQUIRE(structures.size() == 1);
    BOOST_CHECK(structures[0]->isOK());

    // Test a call spread manually and automatically
    std::shared_ptr<EuropeanOption> put = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, structureVolume, SELL, PUT, 0);
    std::shared_ptr<EuropeanOption> purchasedCall = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 115, structureVolume, BUY, CALL, 0);
    std::shared_ptr<EuropeanOption> soldCall = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 130, structureVolume, SELL, CALL, 0);
    vector<std::shared_ptr<SingleInstrumentContract>> threeway;
    threeway.push_back(put);
    threeway.push_back(purchasedCall);
    threeway.push_back(soldCall);
    Structure manualStructure(threeway, regCapFactors);
    structures = Structure::createStructures(threeway, regCapFactors, successful);
    BOOST_REQUIRE(structures.size() == 1);
    std::shared_ptr<Structure> autoStructure = structures[0];
    BOOST_CHECK(autoStructure->isOK());
    BOOST_CHECK(abs(autoStructure->getMtMPlusAddOn(re) - manualStructure.getMtMPlusAddOn(re)) <
                EPSILON_FOR_DOUBLES * structureVolume);

    // Test most complex case
    std::shared_ptr<EuropeanOption> s1p1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 80, structureVolume, BUY, PUT, 0);
    std::shared_ptr<EuropeanOption> s1p2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 85, 2 * structureVolume, BUY, PUT, 0);
    std::shared_ptr<EuropeanOption> s1p3 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 85, structureVolume, BUY, PUT, 0);

    std::shared_ptr<EuropeanOption> s1c1 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 100, 6 * structureVolume, BUY, CALL, 0);
    std::shared_ptr<EuropeanOption> s1c2 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 110, 6 * structureVolume, SELL, CALL, 0);
    std::shared_ptr<EuropeanOption> s1c3 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 110, 6 * structureVolume, SELL, CALL, 0);
    std::shared_ptr<EuropeanOption> s1c4 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 120, 6 * structureVolume, BUY, CALL, 0);

    std::shared_ptr<EuropeanOption> s1c5 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 130, 2 * structureVolume, BUY, CALL, 0);
    std::shared_ptr<EuropeanOption> s1c6 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 130, structureVolume, BUY, CALL, 0);
    std::shared_ptr<EuropeanOption> s1c7 = GenerateContracts::getDefaultEuropeanOption(
        anchorDate, yearsToSettlement, 140, structureVolume, SELL, CALL, 0);

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

    vector<std::shared_ptr<Structure>> s1 = Structure::createStructures(contracts, regCapFactors, successful);

    BOOST_REQUIRE(successful);
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
        BOOST_CHECK(abs(structrePayoff - underlyingPayoff) < 1e-8);
    }
    double v1 = s1[0]->getValue(re);
    double v2 = s1[1]->getValue(re);
    double c = 0;
    for (size_t j = 0; j < contracts.size(); ++j)
    {
        c += contracts[j]->getValue(re);
    }
    BOOST_CHECK(abs(v1 + v2 - c) < 1e-8);
}

test_suite* StructureTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("Structures");

    suite->add(BOOST_TEST_CASE(&StructureTest::testGradient));
    suite->add(BOOST_TEST_CASE(&StructureTest::testIsUncappedOnTheLeft));
    suite->add(BOOST_TEST_CASE(&StructureTest::testGetContractsWithUncappedExposureOnTheLeft));

    suite->add(BOOST_TEST_CASE(&StructureTest::testQualifyForRegulatoryCapitalOffset));
    // suite->add(BOOST_TEST_CASE(&StructureTest::testIsConvex));
    suite->add(BOOST_TEST_CASE(&StructureTest::testMoveContracts));

    suite->add(BOOST_TEST_CASE(&StructureTest::testSettlementDate));

    suite->add(BOOST_TEST_CASE(&StructureTest::testCashFlowN));

    suite->add(BOOST_TEST_CASE(&StructureTest::testEuropeanForward));
    suite->add(BOOST_TEST_CASE(&StructureTest::testAsianforward));
    suite->add(BOOST_TEST_CASE(&StructureTest::testEuropeanOption));
    suite->add(BOOST_TEST_CASE(&StructureTest::testAsianOption));
    suite->add(BOOST_TEST_CASE(&StructureTest::testMixedStructures));
    suite->add(BOOST_TEST_CASE(&StructureTest::testSpecialCases));

    suite->add(BOOST_TEST_CASE(&StructureTest::testStructureCreation));
    return suite;
}
