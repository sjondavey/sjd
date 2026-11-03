#include "PortfolioInsuranceTest.h"
#include <support/GenerateRatesEnvironments.h>

#include <math.h>

using namespace std;
using namespace QuantLib;

using namespace std;
using namespace boost::unit_test_framework;
using namespace sjd;

void PortfolioInsuranceTest::testSimpleInsurance()
{
    BOOST_TEST_MESSAGE("Testing Simple Insurance");

    GenerateRates gr = GenerateRates();

    Date policyStartDate = Date(27, Jan, 2010);
    Date policyMaturityDate = Date(27, Jan, 2020);
    double sumInsured = 50000.0;

    std::shared_ptr<DayCounter> dayCounter = std::make_shared<Actual365Fixed>();
    std::shared_ptr<Calendar> calendar = std::make_shared<NullCalendar>();
    Date anchorDate = Date(27, Jan, 2013);
    size_t years = 10;
    double baseRate = 0.03;
    vector<Date> manditoryDates = vector<Date>();
    std::shared_ptr<DiscountCurve> baseCurve =
        gr.getDiscountCurve(dayCounter, calendar, anchorDate, years, baseRate, manditoryDates);
    double standbyRate = baseRate + 0.01;
    double utilisationRate = baseRate + 0.06;
    std::shared_ptr<DiscountCurve> standbyCurve =
        gr.getDiscountCurve(dayCounter, calendar, anchorDate, years, standbyRate, manditoryDates);
    std::shared_ptr<DiscountCurve> utilisationCurve =
        gr.getDiscountCurve(dayCounter, calendar, anchorDate, years, utilisationRate, manditoryDates);

    std::shared_ptr<SimpleInsurance> si = std::make_shared<SimpleInsurance>(
        policyStartDate, policyMaturityDate, sumInsured, standbyCurve, utilisationCurve);
    BOOST_REQUIRE(si->validate().isOK());

    vector<double> fwdFwdDF, standbyFwdFwd, utilisationFwdFwd, mtm, insurancePayments;
    vector<Date> dates;
    for (size_t i = 0; i < years; ++i)
    {
        Date d = calendar->advance(anchorDate, static_cast<int>(i), Years);
        dates.push_back(d);
    }
    fwdFwdDF.push_back(baseCurve->getDiscountFactor(dates[0]));
    standbyFwdFwd.push_back(standbyCurve->getDiscountFactor(dates[0]));
    utilisationFwdFwd.push_back(utilisationCurve->getDiscountFactor(dates[0]));
    insurancePayments.push_back(0);
    mtm.push_back(years / 2.0 * 20000.0);
    for (size_t i = 1; i < years; ++i)
    {
        fwdFwdDF.push_back(baseCurve->getDiscountFactor(dates[i - 1], dates[i]));
        standbyFwdFwd.push_back(standbyCurve->getDiscountFactor(dates[i - 1], dates[i]));
        utilisationFwdFwd.push_back(utilisationCurve->getDiscountFactor(dates[i - 1], dates[i]));
        mtm.push_back((years / 2.0 - i) * 20000.0);
        insurancePayments.push_back(0);
    }

    si->getInsurancePayments(dates, mtm, insurancePayments);
    vector<double> insurancePaymentsManual;
    double used = min(mtm[0], sumInsured);
    used = max(0.0, used);
    double standby = sumInsured - used;
    double pmnt = used * (1.0 / utilisationFwdFwd[0] - 1) + standby * (1.0 / standbyFwdFwd[0] - 1);
    insurancePaymentsManual.push_back(pmnt);
    for (size_t i = 1; i < mtm.size(); ++i)
    {
        if ((dates[i] >= policyStartDate) && (dates[i] <= policyMaturityDate))
        {
            used = min(mtm[i], sumInsured);
            used = max(0.0, used);
            standby = sumInsured - used;
            pmnt = used * (1.0 / utilisationFwdFwd[i] - 1) + standby * (1.0 / standbyFwdFwd[i] - 1);
            insurancePaymentsManual.push_back(pmnt);
        }
        else
            insurancePaymentsManual.push_back(0);
    }

    BOOST_REQUIRE(insurancePaymentsManual.size() == insurancePayments.size());
    for (size_t i = 0; i < insurancePayments.size(); ++i)
        BOOST_CHECK(abs(insurancePaymentsManual[i] - insurancePayments[i]) < 1e-10);
}

void PortfolioInsuranceTest::testSimpleInsuranceWithTermStructure()
{
    BOOST_TEST_MESSAGE("Testing Simple Insurance with Term Structure");

    GenerateRates gr = GenerateRates();
    Date policyStartDate = Date(27, Jan, 2010);

    std::shared_ptr<DayCounter> dayCounter = std::make_shared<Actual365Fixed>();
    std::shared_ptr<Calendar> calendar = std::make_shared<NullCalendar>();
    size_t years = 10;
    Date policyMaturityDate = calendar->advance(policyStartDate, static_cast<int>(years - 1), Years);
    Date anchorDate = Date(27, Jan, 2013);
    double baseRate = 0.03;
    vector<Date> manditoryDates = vector<Date>();
    std::shared_ptr<DiscountCurve> baseCurve =
        gr.getDiscountCurve(dayCounter, calendar, anchorDate, years, baseRate, manditoryDates);
    double standbyRate = baseRate + 0.01;
    double utilisationRate = baseRate + 0.06;
    std::shared_ptr<DiscountCurve> standbyCurve =
        gr.getDiscountCurve(dayCounter, calendar, anchorDate, years, standbyRate, manditoryDates);
    std::shared_ptr<DiscountCurve> utilisationCurve =
        gr.getDiscountCurve(dayCounter, calendar, anchorDate, years, utilisationRate, manditoryDates);

    vector<Date> insuranceDates;
    vector<double> insuredAmounts;
    for (size_t i = 1; i < years; ++i)
    {
        insuranceDates.push_back(calendar->advance(policyStartDate, static_cast<int>(i), Years));
        insuredAmounts.push_back(100000 - static_cast<int>(i) * 10000);
    }

    std::shared_ptr<SimpleInsuranceWithTermStructure> siWT =
        std::make_shared<SimpleInsuranceWithTermStructure>(policyStartDate, policyMaturityDate,
                                                           insuranceDates, insuredAmounts, standbyCurve,
                                                           utilisationCurve);
    BOOST_REQUIRE(siWT->validate().isOK());

    int index = (anchorDate - policyStartDate) / 366;
    BOOST_CHECK(abs(siWT->getSumInsured(anchorDate) - insuredAmounts[index]) < 1e-10);
    BOOST_CHECK(abs(siWT->getSumInsured(insuranceDates[1]) - insuredAmounts[1]) < 1e-10);
    BOOST_CHECK(abs(siWT->getSumInsured(insuranceDates[years - 2]) - insuredAmounts[years - 2]) < 1e-10);
    BOOST_CHECK(abs(siWT->getSumInsured(insuranceDates[years - 2] - 1) - insuredAmounts[years - 2]) < 1e-10);

    vector<double> fwdFwdDF, standbyFwdFwd, utilisationFwdFwd, mtm, insurancePayments;
    vector<Date> fwdDates;
    for (size_t i = 0; i < years; ++i)
    {
        Date d = calendar->advance(anchorDate, static_cast<int>(i), Years);
        fwdDates.push_back(d);
    }
    fwdFwdDF.push_back(baseCurve->getDiscountFactor(fwdDates[0]));
    standbyFwdFwd.push_back(standbyCurve->getDiscountFactor(fwdDates[0]));
    utilisationFwdFwd.push_back(utilisationCurve->getDiscountFactor(fwdDates[0]));
    insurancePayments.push_back(0);
    mtm.push_back(years / 2.0 * 20000.0);
    for (size_t i = 1; i < years; ++i)
    {
        fwdFwdDF.push_back(baseCurve->getDiscountFactor(fwdDates[i - 1], fwdDates[i]));
        standbyFwdFwd.push_back(standbyCurve->getDiscountFactor(fwdDates[i - 1], fwdDates[i]));
        utilisationFwdFwd.push_back(utilisationCurve->getDiscountFactor(fwdDates[i - 1], fwdDates[i]));
        mtm.push_back((years / 2.0 - i) * 20000.0);
        insurancePayments.push_back(0);
    }

    siWT->getInsurancePayments(fwdDates, mtm, insurancePayments);
    vector<double> insurancePaymentsManual;

    double sumInsured = siWT->getSumInsured(fwdDates[0]);

    double used = min(mtm[0], sumInsured);
    used = max(0.0, used);
    double standby = sumInsured - used;
    double pmnt = used * (1.0 / utilisationFwdFwd[0] - 1) + standby * (1.0 / standbyFwdFwd[0] - 1);
    insurancePaymentsManual.push_back(pmnt);
    for (size_t i = 1; i < mtm.size(); ++i)
    {
        if ((fwdDates[i] >= policyStartDate) && (fwdDates[i] <= policyMaturityDate))
        {

            sumInsured = siWT->getSumInsured(fwdDates[i]);

            used = min(mtm[i], sumInsured);
            used = max(0.0, used);
            standby = sumInsured - used;
            pmnt = used * (1.0 / utilisationFwdFwd[i] - 1) + standby * (1.0 / standbyFwdFwd[i] - 1);
            insurancePaymentsManual.push_back(pmnt);
        }
        else
        {
            insurancePaymentsManual.push_back(0);
        }
    }

    BOOST_REQUIRE(insurancePaymentsManual.size() == insurancePayments.size());
    for (size_t i = 0; i < insurancePayments.size(); ++i)
    {
        BOOST_CHECK(abs(insurancePaymentsManual[i] - insurancePayments[i]) < 1e-10);
    }
}

test_suite* PortfolioInsuranceTest::suite()
{

    test_suite* suite = BOOST_TEST_SUITE("Insurance");
    suite->add(BOOST_TEST_CASE(&PortfolioInsuranceTest::testSimpleInsurance));
    suite->add(BOOST_TEST_CASE(&PortfolioInsuranceTest::testSimpleInsuranceWithTermStructure));

    return suite;
}
