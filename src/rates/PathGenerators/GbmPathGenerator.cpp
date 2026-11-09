#include <cmath>
#include <random>
#include <sjd/rates/PathGenerators/GbmPathGenerator.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    Mt19937Normals::Mt19937Normals(long seed) : rng(static_cast<std::mt19937::result_type>(seed)) {}

    double Mt19937Normals::next()
    {
        // Box-Muller. The uniform is in (0, 1) so the log argument is never zero.
        constexpr double twoPi = 6.28318530717958647692;
        const double u1 = (static_cast<double>(rng()) + 0.5) * (1.0 / 4294967296.0);
        const double u2 = (static_cast<double>(rng()) + 0.5) * (1.0 / 4294967296.0);
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(twoPi * u2);
    }

    void Mt19937Normals::discardNormals(std::uint64_t count)
    {
        rng.discard(count * 2);
    }

    /*======================================================================================
    EconomicScenarioGenerator
    =======================================================================================*/
    std::string EconomicScenarioGenerator::validationRoot() const
    {
        return "path_generator";
    }

    void EconomicScenarioGenerator::runChecks()
    {
        resetErrorMessages();
        if (re == 0)
        {
            recordIssue(issues::PATH_GENERATOR_RATES_MISSING, "Rates environment not set", {});
            return;
        }
        if (!re->validate().isOK())
        {
            absorbChildReport(re->validationReport(), {});
        }
        if ((simulationDates.size() > 0) && (simulationDates.front() < anchorDate))
        {
            recordIssue(issues::PATH_GENERATOR_DATES_BEFORE_ANCHOR, "Simulation dates before anchor date",
                        {"simulation_dates"});
            return;
        }
        if ((simulationDates.size() > 0) && (!re->isInRange(simulationDates.back())))
        {
            recordIssue(issues::PATH_GENERATOR_DATES_AFTER_END, "Simulation dates after rate source end",
                        {"simulation_dates"});
            return;
        }
        return;
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
    GbmPathGenerator
    =======================================================================================*/
    GbmPathGenerator::GbmPathGenerator(const std::shared_ptr<RatesEnvironment> re)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("GbmPathGenerator");
        setBaseRates(re);
        // random_device, not the clock, so two generators built in the same second differ,
        // and the value is the one setRandomSeed stored.
        setRandomSeed(static_cast<long>(std::random_device{}()));
    }

    void GbmPathGenerator::setBaseRates(const std::shared_ptr<RatesEnvironment> rates)
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

    void GbmPathGenerator::runChecks()
    {
        EconomicScenarioGenerator::runChecks();
        if (hasErrors())
        {
            return;
        }
        if (drs == 0)
        {
            recordIssue(
                issues::PATH_GENERATOR_DISCOUNT_NOT_DETERMINISTIC,
                "Rates environment does not contain a discounting rate source that rolls deterministically",
                {"discount_rate_source"});
        }
        if (foreigndrs == 0 && (re->hasForeignDRS())) // foreign DRS may not be required
        {
            recordIssue(issues::PATH_GENERATOR_FOREIGN_DISCOUNT_NOT_DETERMINISTIC,
                        "Rates environment does not contain a foreign discounting rate source that rolls "
                        "deterministically",
                        {"foreign_discount_rate_source"});
        }
        if (vrs == 0)
        {
            recordIssue(
                issues::PATH_GENERATOR_VOLATILITY_NOT_DETERMINISTIC,
                "Rates environment does not contain a volatility rate source that rolls deterministically",
                {"volatility_rate_source"});
        }
        if (frs == 0)
        {
            recordIssue(issues::PATH_GENERATOR_FORWARD_NOT_STOCHASTIC,
                        "Rates environment does not contain a forward rate source that rolls stochastically",
                        {"forward_rate_source"});
        }
        if (frsDependsOnInterestRateDifferential && foreigndrs == 0)
        {
            recordIssue(issues::PATH_GENERATOR_FOREIGN_DISCOUNT_REQUIRED,
                        "FRS requires a foreign interest rate source but rates environment does not contain "
                        "one that rolls deterministically",
                        {"foreign_discount_rate_source"});
        }
        if (re->hasFixingRS() && (fixingrs == 0))
        {
            recordIssue(issues::PATH_GENERATOR_FIXING_NOT_ROLLING,
                        "Rates environment does not contain a fixing rate source that rolls",
                        {"fixing_rate_source"});
        }
        return;
    }

    bool GbmPathGenerator::setSimulationDates(set<Date> simulationDatesSet)
    {
        EconomicScenarioGenerator::setSimulationDates(simulationDatesSet);
        if (validate().isOK())
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

    void GbmPathGenerator::setRandomSeed(long seedInput)
    {
        seed = seedInput;
        normals = Mt19937Normals(seedInput);
    }

    double GbmPathGenerator::drawStandardNormal()
    {
        return normals.next();
    }

    long GbmPathGenerator::getRandomSeed() const
    {
        return seed;
    }

    size_t GbmPathGenerator::factors() const
    {
        return 1;
    }

    PathWorkspace GbmPathGenerator::workspace()
    {
        PathWorkspace space;
        space.curve = frs->cloneCurve();
        space.usesInterestDifferential = frsDependsOnInterestRateDifferential;
        set<Date> dates(simulationDates.begin(), simulationDates.end());
        space.curve->setForwardValuationDates(dates);
        space.shocks.assign(numDates, 0.0);
        space.curve->setRandomVariables(space.shocks);
        space.forwards = space.curve->rollForward();

        if (fixingrs != 0)
        {
            space.fixingDates = fixingrs->getFixingDatesFor(simulationDates);
            space.fixingRates.assign(space.fixingDates.size(), 0.0);
        }

        vector<std::shared_ptr<RatesEnvironment>> ratesPath;
        for (size_t i = 0; i < numDates; ++i)
        {
            std::shared_ptr<RatesEnvironment> environment =
                std::make_shared<RatesEnvironment>(simulationDates[i]);
            environment->setDRS(drss[i]);
            if (foreigndrs != 0)
            {
                environment->setForeignDRS(foreigndrss[i]);
                if (space.usesInterestDifferential)
                {
                    std::shared_ptr<UsesInterestRateDifferential> fx =
                        std::dynamic_pointer_cast<UsesInterestRateDifferential>(space.forwards[i]);
                    if (fx != 0)
                    {
                        fx->setRateSources(drss[i], foreigndrss[i]);
                    }
                }
            }
            environment->setFRS(space.forwards[i]);
            environment->setVRS(vrss[i]);
            if (fixingrs != 0)
            {
                std::shared_ptr<RollingFixingRateSource> rolled =
                    fixingrs->rollForward(simulationDates[i], space.fixingDates, space.fixingRates);
                space.fixings.push_back(rolled);
                environment->setFixingRS(rolled);
            }
            ratesPath.push_back(environment);
        }
        space.path = std::make_shared<Path>(ratesPath);
        return space;
    }

    std::shared_ptr<Path> GbmPathGenerator::generatePath(Mt19937Normals& rng, PathWorkspace& space)
    {
        space.shocks.resize(numDates);
        for (size_t i = 0; i < numDates; ++i)
        {
            space.shocks[i] = rng.next();
        }
        return writeShocks(space.shocks, space);
    }

    std::shared_ptr<Path> GbmPathGenerator::generateAntithetic(PathWorkspace& space)
    {
        for (size_t i = 0; i < space.shocks.size(); ++i)
        {
            space.shocks[i] = -space.shocks[i];
        }
        return writeShocks(space.shocks, space);
    }

    std::shared_ptr<Path> GbmPathGenerator::writeShocks(const vector<double>& shocks, PathWorkspace& space)
    {
        space.shocks = shocks;
        space.curve->setRandomVariables(space.shocks);
        if (space.fixings.empty())
        {
            space.forwards = space.curve->rollForward();
        }
        else
        {
            space.forwards = space.curve->rollForward(space.fixingDates, space.fixingRates);
            for (size_t i = 0; i < space.fixings.size(); ++i)
            {
                space.fixings[i]->updateAppendedRates(space.fixingRates);
            }
        }
        for (size_t i = 0; i < numDates; ++i)
        {
            space.path->getRatesEnvironment(i)->setFRS(space.forwards[i]);
        }
        return space.path;
    }

    std::shared_ptr<Path> GbmPathGenerator::generateSpot(const vector<double>& spot, PathWorkspace& space)
    {
        return writeShocks(space.curve->getImpliedRandomNumbers(spot), space);
    }

    std::shared_ptr<Path> GbmPathGenerator::getConfidenceInterval(double confidenceInterval)
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

    std::shared_ptr<Path> GbmPathGenerator::getRandomPath()
    {
        for (size_t i = 0; i < numDates; ++i)
        {
            randomNumbers[i] = drawStandardNormal();
        }
        return getPathForRandomNumber(randomNumbers);
    }

    std::shared_ptr<Path> GbmPathGenerator::getAntitheticPath()
    {
        for (size_t i = 0; i < numDates; ++i)
        {
            randomNumbers[i] = -randomNumbers[i];
        }
        return getPathForRandomNumber(randomNumbers);
    }

    std::shared_ptr<Path> GbmPathGenerator::getPathForRandomNumber(vector<double> draws)
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

    std::shared_ptr<Path> GbmPathGenerator::getPathForSpot(vector<double> spot)
    {
        vector<double> impliedRV = frs->getImpliedRandomNumbers(spot);
        return getPathForRandomNumber(impliedRV);
    }

    void GbmPathGenerator::updateFXForwardWithRolledDiscountRateSources(size_t stepNumber)
    {
        std::shared_ptr<sjd::UsesInterestRateDifferential> tmpFRS =
            std::dynamic_pointer_cast<sjd::UsesInterestRateDifferential>(frss[stepNumber]);
        if (tmpFRS != 0)
        {
            tmpFRS->setRateSources(drss[stepNumber], foreigndrss[stepNumber]);
        }
    }

}
