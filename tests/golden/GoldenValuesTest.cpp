#include "GoldenValuesTest.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <sjd/contracts/ContractStressor.h>
#include <sjd/contracts/Portfolio/Insurance/PortfolioInsurance.h>
#include <support/GenerateContracts.h>
#include <support/GenerateRatesEnvironments.h>

using namespace boost::unit_test_framework;
using namespace QuantLib;
using namespace sjd;

namespace
{
    using Named = std::vector<std::pair<std::string, double>>;

    const double relativeTolerance = 1e-9;

    bool printMode()
    {
        return std::getenv("SJD_PRINT_GOLDEN") != nullptr;
    }

    void print(const std::string& table, const Named& actual)
    {
        std::cout << "// " << table << "\n";
        for (const auto& entry : actual)
        {
            std::cout << "        {\"" << entry.first << "\", " << std::setprecision(17) << entry.second
                      << "},\n";
        }
    }

    void compare(const std::string& table, const Named& expected, const Named& actual)
    {
        if (printMode())
        {
            print(table, actual);
            return;
        }
        BOOST_REQUIRE_EQUAL(expected.size(), actual.size());
        for (size_t i = 0; i < expected.size(); ++i)
        {
            BOOST_CHECK_EQUAL(expected[i].first, actual[i].first);
            const double scale = std::max(1.0, std::abs(expected[i].second));
            BOOST_CHECK_MESSAGE(std::abs(expected[i].second - actual[i].second) <= relativeTolerance * scale,
                                table << " " << actual[i].first << ": expected " << std::setprecision(17)
                                      << expected[i].second << ", got " << actual[i].second);
        }
    }

    Named stressedValues(const std::string& prefix,
                         std::shared_ptr<Contract> contract,
                         std::shared_ptr<RatesEnvironment> re)
    {
        ContractStressor stressor(contract, re);
        BOOST_REQUIRE_MESSAGE(stressor.isOK(), stressor.getErrorMessagesAsString());
        Named values;
        for (const auto& stressed : stressor.getStressedValues())
        {
            values.emplace_back(prefix + ": " + stressed.first, stressed.second);
        }
        return values;
    }
}

void GoldenValuesTest::testContractStressor()
{
    BOOST_TEST_MESSAGE("Testing golden values for ContractStressor");

    Date anchorDate = Date(27, Jan, 2013);
    GenerateRates gr = GenerateRates();
    std::shared_ptr<RatesEnvironment> re = gr.getRatesEnvironmentForAsianOption(anchorDate, std::vector<Date>(0));

    Named actual;
    for (const auto& part :
         {stressedValues("european call", GenerateContracts::getDefaultEuropeanOption(anchorDate, 1, 100, 1000, BUY, CALL, 2.5), re),
          stressedValues("european put", GenerateContracts::getDefaultEuropeanOption(anchorDate, 2, 95, 1000, SELL, PUT, 0), re),
          stressedValues("european forward", GenerateContracts::getDefaultEuropeanForward(anchorDate, 1, 102, 1000, BUY), re),
          stressedValues("asian put", GenerateContracts::getDefaultAsianOption(anchorDate, 1, 20, 98, 1000, BUY, PUT, 1.0), re)})
    {
        actual.insert(actual.end(), part.begin(), part.end());
    }

    const Named expected = {
        {"european call: Base", 10229.753408584987},
        {"european call: Domestic DF + 1% NACA a/360", 10109.83509049577},
        {"european call: Domestic ZC Stress base", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 3m", 10229.753408584987},
        {"european call: Domestic DF 1 bp ZC Stress at 6m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 9m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 12m", 10231.111875707797},
        {"european call: Domestic DF 1 bp ZC Stress at 15m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 18m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 21m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 24m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 27m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 30m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 33m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 36m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 39m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 42m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 45m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 48m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 51m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 54m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 57m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 60m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 63m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 66m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 69m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 72m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 75m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 78m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 81m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 84m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 87m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 90m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 93m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 96m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 99m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 102m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 105m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 108m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 111m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 114m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 117m", 10229.753408584984},
        {"european call: Domestic DF 1 bp ZC Stress at 120m", 10229.753408584984},
        {"european call: Vol + 1% ", 10600.974599345305},
        {"european put: Base", -8916.9519833232262},
        {"european put: Domestic DF + 1% NACA a/360", -8749.7422582444433},
        {"european put: Domestic ZC Stress base", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 3m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 6m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 9m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 12m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 15m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 18m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 21m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 24m", -8917.9674711243606},
        {"european put: Domestic DF 1 bp ZC Stress at 27m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 30m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 33m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 36m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 39m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 42m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 45m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 48m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 51m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 54m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 57m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 60m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 63m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 66m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 69m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 72m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 75m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 78m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 81m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 84m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 87m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 90m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 93m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 96m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 99m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 102m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 105m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 108m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 111m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 114m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 117m", -8916.9519833232262},
        {"european put: Domestic DF 1 bp ZC Stress at 120m", -8916.9519833232262},
        {"european put: Vol + 1% ", -9381.4904497904063},
        {"european forward: Base", 3644.6304343476904},
        {"european forward: Domestic DF + 1% NACA a/360", 3610.2968586910715},
        {"european forward: Domestic ZC Stress base", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 3m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 6m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 9m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 12m", 3645.0193743735454},
        {"european forward: Domestic DF 1 bp ZC Stress at 15m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 18m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 21m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 24m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 27m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 30m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 33m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 36m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 39m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 42m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 45m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 48m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 51m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 54m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 57m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 60m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 63m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 66m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 69m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 72m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 75m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 78m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 81m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 84m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 87m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 90m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 93m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 96m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 99m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 102m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 105m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 108m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 111m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 114m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 117m", 3644.6304343476904},
        {"european forward: Domestic DF 1 bp ZC Stress at 120m", 3644.6304343476904},
        {"european forward: Vol + 1% ", 3644.6304343476904},
        {"asian put: Base", 5234.6883901471829},
        {"asian put: Domestic DF + 1% NACA a/360", 5175.9556463492872},
        {"asian put: Domestic ZC Stress base", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 3m", 5234.6883901471829},
        {"asian put: Domestic DF 1 bp ZC Stress at 6m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 9m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 12m", 5235.3537305451573},
        {"asian put: Domestic DF 1 bp ZC Stress at 15m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 18m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 21m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 24m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 27m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 30m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 33m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 36m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 39m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 42m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 45m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 48m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 51m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 54m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 57m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 60m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 63m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 66m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 69m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 72m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 75m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 78m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 81m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 84m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 87m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 90m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 93m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 96m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 99m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 102m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 105m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 108m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 111m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 114m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 117m", 5234.688390147182},
        {"asian put: Domestic DF 1 bp ZC Stress at 120m", 5234.688390147182},
        {"asian put: Vol + 1% ", 5586.4117340588291},
    };
    compare("contract stressor", expected, actual);
}

void GoldenValuesTest::testSimpleInsurance()
{
    BOOST_TEST_MESSAGE("Testing golden values for SimpleInsurance");

    GenerateRates gr = GenerateRates();
    std::shared_ptr<DayCounter> dayCounter = std::make_shared<Actual365Fixed>();
    std::shared_ptr<Calendar> calendar = std::make_shared<NullCalendar>();
    Date anchorDate = Date(27, Jan, 2013);
    size_t years = 10;
    std::shared_ptr<DiscountCurve> standbyCurve =
        gr.getDiscountCurve(dayCounter, calendar, anchorDate, years, 0.04, std::vector<Date>());
    std::shared_ptr<DiscountCurve> utilisationCurve =
        gr.getDiscountCurve(dayCounter, calendar, anchorDate, years, 0.09, std::vector<Date>());

    SimpleInsurance insurance(Date(27, Jan, 2010), Date(27, Jan, 2020), 50000.0, standbyCurve, utilisationCurve);
    BOOST_REQUIRE(insurance.isOK());

    std::vector<Date> dates;
    std::vector<double> mtm;
    for (size_t i = 0; i < years; ++i)
    {
        dates.push_back(calendar->advance(anchorDate, static_cast<Integer>(i), Years));
        mtm.push_back((years / 2.0 - i) * 20000.0);
    }
    std::vector<double> payments(years, 0.0);
    insurance.getInsurancePayments(dates, mtm, payments);

    Named actual;
    for (size_t i = 0; i < payments.size(); ++i)
    {
        actual.emplace_back("payment " + std::to_string(i), payments[i]);
    }

    const Named expected = {
        {"payment 0", 0},
        {"payment 1", 4708.7141852605537},
        {"payment 2", 4708.7141852604764},
        {"payment 3", 4175.0790901323207},
        {"payment 4", 3116.6275243026666},
        {"payment 5", 2040.5387096194215},
        {"payment 6", 2040.5387096194106},
        {"payment 7", 2040.5387096193995},
        {"payment 8", 0},
        {"payment 9", 0},
    };
    compare("simple insurance", expected, actual);
}

test_suite* GoldenValuesTest::suite()
{
    test_suite* suite = BOOST_TEST_SUITE("GoldenValues");
    suite->add(BOOST_TEST_CASE(&GoldenValuesTest::testContractStressor));
    suite->add(BOOST_TEST_CASE(&GoldenValuesTest::testSimpleInsurance));
    return suite;
}
