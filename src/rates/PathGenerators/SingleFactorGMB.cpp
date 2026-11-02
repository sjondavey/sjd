#include <random>
#include <sjd/rates/PathGenerators/SingleFactorGMB.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    /*======================================================================================
    EconomicScenarioGenerator
    =======================================================================================*/
    std::string EconomicScenarioGenerator::validationRoot() const
    {
        return "path_generator";
    }

    bool EconomicScenarioGenerator::isOK()
    {
        resetErrorMessages();
        if (re == 0)
        {
            recordIssue("PATH_GENERATOR_RATES_MISSING", "Rates environment not set", {});
            return false;
        }
        if (!re->isOK())
        {
            absorbChildReport(re->validationReport(), {});
        }
        if ((simulationDates.size() > 0) && (simulationDates.front() < anchorDate))
        {
            recordIssue("PATH_GENERATOR_DATES_BEFORE_ANCHOR", "Simulation dates before anchor date",
                        {"simulation_dates"});
            return false;
        }
        if ((simulationDates.size() > 0) && (!re->isInRange(simulationDates.back())))
        {
            recordIssue("PATH_GENERATOR_DATES_AFTER_END", "Simulation dates after rate source end",
                        {"simulation_dates"});
            return false;
        }
        return !hasErrors();
    }

    Date EconomicScenarioGenerator::getAnchorDate() const
    {
        return anchorDate;
    }

    void EconomicScenarioGenerator::setBaseRates(const std::shared_ptr<RatesEnvironment> reInput)
    {
        re = reInput;
        anchorDate = re->getAnchorDate();
    }

    std::shared_ptr<RatesEnvironment> EconomicScenarioGenerator::getBaseRates()
    {
        return re;
    }

    bool EconomicScenarioGenerator::setSimulationDates(set<Date> simulationDatesSet)
    {
        simulationDates = vector<Date>(simulationDatesSet.begin(), simulationDatesSet.end());
        numDates = simulationDates.size();
        return true;
    }

    /*======================================================================================
    SingleFactorEconomicScenarioGenerator
    =======================================================================================*/
    SingleFactorEconomicScenarioGenerator::SingleFactorEconomicScenarioGenerator(
        const std::shared_ptr<RatesEnvironment> re)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("SingleFactorEconomicScenarioGenerator");
        setBaseRates(re);
        // random_device, not the clock, so two generators built in the same second differ,
        // and the value is the one setRandomSeed stored.
        setRandomSeed(static_cast<long>(std::random_device{}()));
    }

    void SingleFactorEconomicScenarioGenerator::setBaseRates(const std::shared_ptr<RatesEnvironment> rates)
    {
        EconomicScenarioGenerator::setBaseRates(rates);
        frsDependsOnInterestRateDifferential = false;
        if (rates->hasDRS())
        {
            drs = std::dynamic_pointer_cast<sjd::DeterministicDiscountRateSource>(rates->getDRS());
        }
        if (rates->hasForeignDRS())
        {
            foreigndrs =
                std::dynamic_pointer_cast<sjd::DeterministicDiscountRateSource>(rates->getForeignDRS());
        }
        if (rates->hasVRS())
        {
            vrs = std::dynamic_pointer_cast<sjd::DeterministicVolatilityRateSource>(rates->getVRS());
        }
        if (rates->hasFRS())
        {
            frs = std::dynamic_pointer_cast<sjd::ForwardRateSourceStochastic>(rates->getFRS());
            std::shared_ptr<sjd::UsesInterestRateDifferential> tmpFRS =
                std::dynamic_pointer_cast<sjd::UsesInterestRateDifferential>(frs);
            if (tmpFRS != 0)
            {
                frsDependsOnInterestRateDifferential = true;
            }
        }
        if (rates->hasFixingRS())
        {
            fixingrs = std::dynamic_pointer_cast<sjd::RollingFixingRateSource>(rates->getFixingRS());
        }
    }

    bool SingleFactorEconomicScenarioGenerator::isOK()
    {
        if (!EconomicScenarioGenerator::isOK())
        {
            return false;
        }
        if (drs == 0)
        {
            recordIssue(
                "PATH_GENERATOR_DISCOUNT_NOT_DETERMINISTIC",
                "Rates environment does not contain a discounting rate source that rolls deterministically",
                {"discount_rate_source"});
        }
        if (foreigndrs == 0 && (re->hasForeignDRS())) // foreign DRS may not be required
        {
            recordIssue("PATH_GENERATOR_FOREIGN_DISCOUNT_NOT_DETERMINISTIC",
                        "Rates environment does not contain a foreign discounting rate source that rolls "
                        "deterministically",
                        {"foreign_discount_rate_source"});
        }
        if (vrs == 0)
        {
            recordIssue(
                "PATH_GENERATOR_VOLATILITY_NOT_DETERMINISTIC",
                "Rates environment does not contain a volatility rate source that rolls deterministically",
                {"volatility_rate_source"});
        }
        if (frs == 0)
        {
            recordIssue("PATH_GENERATOR_FORWARD_NOT_STOCHASTIC",
                        "Rates environment does not contain a forward rate source that rolls stochastically",
                        {"forward_rate_source"});
        }
        if (frsDependsOnInterestRateDifferential && foreigndrs == 0)
        {
            recordIssue("PATH_GENERATOR_FOREIGN_DISCOUNT_REQUIRED",
                        "FRS requires a foreign interest rate source but rates environment does not contain "
                        "one that rolls deterministically",
                        {"foreign_discount_rate_source"});
        }
        if (re->hasFixingRS() && (fixingrs == 0))
        {
            recordIssue("PATH_GENERATOR_FIXING_NOT_ROLLING",
                        "Rates environment does not contain a fixing rate source that rolls",
                        {"fixing_rate_source"});
        }
        return !errorTracking->getHasErrors();
    }

    bool SingleFactorEconomicScenarioGenerator::setSimulationDates(set<Date> simulationDatesSet)
    {
        EconomicScenarioGenerator::setSimulationDates(simulationDatesSet);
        if (isOK())
        {
            drs->setForwardValuationDates(simulationDatesSet);
            drss = drs->rollForward();

            if (foreigndrs != 0)
            {
                foreigndrs->setForwardValuationDates(simulationDatesSet);
                foreigndrss = foreigndrs->rollForward();
            }

            vrs->setForwardValuationDates(simulationDatesSet);
            vrss = vrs->rollForward();

            frs->setForwardValuationDates(simulationDatesSet);
            // fill it with something
            randomNumbers = vector<double>(numDates, 0);
            frs->setRandomVariables(randomNumbers);
            frss = frs->rollForward();

            fixingrss.clear();

            vector<std::shared_ptr<RatesEnvironment>> ratesPath;
            for (size_t i = 0; i < numDates; ++i)
            {
                std::shared_ptr<RatesEnvironment> tmpRE =
                    std::make_shared<RatesEnvironment>(simulationDates[i]);
                tmpRE->setDRS(drss[i]);
                if (foreigndrs != 0)
                {
                    tmpRE->setForeignDRS(foreigndrss[i]);
                    if (frsDependsOnInterestRateDifferential)
                    {
                        updateFXForwardWithRolledDiscountRateSources(i);
                    }
                }
                tmpRE->setFRS(frss[i]);
                tmpRE->setVRS(vrss[i]);
                if (re->hasFixingRS())
                {
                    allFixingDates = fixingrs->getFixingDatesFor(simulationDates);
                    allFixingRates = vector<double>(allFixingDates.size(), 0);
                    std::shared_ptr<RollingFixingRateSource> rolledfixingrs =
                        fixingrs->rollForward(simulationDates[i], allFixingDates, allFixingRates);
                    fixingrss.push_back(rolledfixingrs);
                    tmpRE->setFixingRS(rolledfixingrs);
                }
                ratesPath.push_back(tmpRE);
            }
            path = std::make_shared<Path>(ratesPath);
            return true;
        }
        return false;
    }

    void SingleFactorEconomicScenarioGenerator::setRandomSeed(long seedInput)
    {
        seed = seedInput;
        rng.seed(static_cast<std::mt19937::result_type>(seedInput));
        nd.reset();
    }

    long SingleFactorEconomicScenarioGenerator::getRandomSeed() const
    {
        return seed;
    }

    std::shared_ptr<Path> SingleFactorEconomicScenarioGenerator::getConfidenceInterval(
        double confidenceInterval)
    {
        boost::math::normal dist(0.0, 1.0);
        double epsi = quantile(dist, confidenceInterval);

        frs->setRandomVariables(vector<double>(numDates, epsi));
        if (fixingrs == 0)
        {
            for (size_t i = 0; i < numDates; ++i)
            {
                frss[i] = frs->rollForward(simulationDates[i], epsi);
                if (frsDependsOnInterestRateDifferential)
                {
                    updateFXForwardWithRolledDiscountRateSources(i);
                }
                path->getRatesEnvironment(i)->setFRS(frss[i]);
            }
        }
        else
        {
            vector<Date>::iterator endIt;
            for (size_t i = 0; i < numDates; ++i)
            {
                endIt = upper_bound(allFixingDates.begin(), allFixingDates.end(), simulationDates[i]);
                vector<Date> datesSubset = vector<Date>(allFixingDates.begin(), endIt);
                vector<double> rateSets = vector<double>(datesSubset.size());
                frss[i] = frs->rollForward(simulationDates[i], epsi, datesSubset, rateSets);
                if (frsDependsOnInterestRateDifferential)
                {
                    updateFXForwardWithRolledDiscountRateSources(i);
                }
                fixingrss[i]->updateAppendedRates(rateSets);
                path->getRatesEnvironment(i)->setFRS(frss[i]);
                path->getRatesEnvironment(i)->setFixingRS(fixingrss[i]);
            }
        }
        return path;
    }

    std::shared_ptr<Path> SingleFactorEconomicScenarioGenerator::getRandomPath()
    {
        for (size_t i = 0; i < numDates; ++i)
        {
            randomNumbers[i] = nd(rng);
        }
        return getPathForRandomNumber(randomNumbers);
    }

    std::shared_ptr<Path> SingleFactorEconomicScenarioGenerator::getAntitheticPath()
    {
        for (size_t i = 0; i < numDates; ++i)
        {
            randomNumbers[i] = -randomNumbers[i];
        }
        return getPathForRandomNumber(randomNumbers);
    }

    std::shared_ptr<Path> SingleFactorEconomicScenarioGenerator::getPathForRandomNumber(vector<double> draws)
    {
        frs->setRandomVariables(draws);
        if (fixingrs == 0)
        {
            frss = frs->rollForward();
        }
        else
        {
            frss = frs->rollForward(allFixingDates, allFixingRates);
            for (size_t i = 0; i < fixingrss.size(); ++i)
            {
                fixingrss[i]->updateAppendedRates(allFixingRates);
            }
        }

        for (size_t i = 0; i < numDates; ++i)
        {
            path->getRatesEnvironment(i)->setFRS(frss[i]);
        }
        return path;
    }

    std::shared_ptr<Path> SingleFactorEconomicScenarioGenerator::getPathForSpot(vector<double> spot)
    {
        vector<double> impliedRV = frs->getImpliedRandomNumbers(spot);
        return getPathForRandomNumber(impliedRV);
    }

    void SingleFactorEconomicScenarioGenerator::updateFXForwardWithRolledDiscountRateSources(
        size_t stepNumber)
    {
        std::shared_ptr<sjd::UsesInterestRateDifferential> tmpFRS =
            std::dynamic_pointer_cast<sjd::UsesInterestRateDifferential>(frss[stepNumber]);
        if (tmpFRS != 0)
        {
            tmpFRS->setRateSources(drss[stepNumber], foreigndrss[stepNumber]);
        }
    }

}