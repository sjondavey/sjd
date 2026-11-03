#include <sjd/contracts/ContractStressor.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    ContractStressor::ContractStressor(std::shared_ptr<Contract> contractInput,
                                       std::shared_ptr<RatesEnvironment> rates)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("ContractStressor");
        contract = contractInput;
        baseRates = rates;
    }

    void ContractStressor::runChecks()
    {
        errorTracking->clear();
        if (!baseRates->validate().isOK())
        {
            errorTracking->absorb(baseRates->validationReport(), {"contract_stressor", "rates_environment"});
            return;
        }
        contract->validate(baseRates);
        if (!contract->isOK())
        {
            errorTracking->absorb(contract->validationReport(), {"contract_stressor", "contract"});
            return;
        }
        return;
    }

    std::string ContractStressor::validationRoot() const
    {
        return "contract_stressor";
    }

    vector<pair<string, double>> ContractStressor::getStressedValues()
    {
        vector<pair<string, std::shared_ptr<RatesEnvironment>>> namedRatesEnvironments =
            getAllRatesEnvironments();
        vector<pair<string, double>> stressedValuations;
        for (size_t i = 0; i < namedRatesEnvironments.size(); ++i)
        {
            double value = contract->getValue(namedRatesEnvironments[i].second);
            stressedValuations.push_back(make_pair(namedRatesEnvironments[i].first, value));
        }
        return stressedValuations;
    }

    vector<pair<string, std::shared_ptr<RatesEnvironment>>> ContractStressor::getAllRatesEnvironments()
    {
        vector<pair<string, std::shared_ptr<RatesEnvironment>>> stressedMarketRates;
        stressedMarketRates.push_back(make_pair("Base", baseRates));

        std::shared_ptr<ForwardRateSource> baseFRS = baseRates->getFRS();
        std::shared_ptr<DiscountRateSource> baseDRS = baseRates->getDRS();
        std::shared_ptr<DiscountRateSource> baseForeignDRS = baseRates->getForeignDRS();
        std::shared_ptr<VolatilityRateSource> baseVRS = baseRates->getVRS();
        std::shared_ptr<FixingRateSource> baseFixingRS = baseRates->getFixingRS();

        std::shared_ptr<sjd::UsesInterestRateDifferential> irdFRS =
            std::dynamic_pointer_cast<sjd::UsesInterestRateDifferential>(baseFRS);
        bool frsUsesIRD = false;
        if (irdFRS != 0 && baseRates->hasForeignDRS() && baseRates->hasDRS())
        {
            frsUsesIRD = true;
        }

        if (baseRates->hasDRS())
        {
            std::shared_ptr<IHaveMarketRateStresses> stressableDRS =
                std::dynamic_pointer_cast<IHaveMarketRateStresses>(baseDRS);
            if (stressableDRS != 0)
            {
                vector<pair<string, std::shared_ptr<RateSource>>> stressedDRSS =
                    stressableDRS->getMarketRateStresses();
                for (size_t i = 0; i < stressedDRSS.size(); ++i)
                {
                    std::shared_ptr<RatesEnvironment> re =
                        std::make_shared<RatesEnvironment>(baseRates->getAnchorDate());
                    std::shared_ptr<DiscountRateSource> stressDRS =
                        std::dynamic_pointer_cast<DiscountRateSource>(stressedDRSS[i].second);
                    re->setDRS(stressDRS);
                    if (baseRates->hasFRS())
                    {
                        if (frsUsesIRD)
                        {
                            Date anchorDate = baseFRS->getAnchorDate();
                            double spotAtT0 = irdFRS->getSpotAtT0();
                            std::shared_ptr<ForwardCurveFX> stressFRS = std::make_shared<ForwardCurveFX>(
                                anchorDate, spotAtT0, stressDRS, baseForeignDRS);
                            re->setFRS(stressFRS);
                        }
                        else
                        {
                            re->setFRS(baseFRS);
                        }
                    }
                    if (baseRates->hasForeignDRS())
                    {
                        re->setForeignDRS(baseForeignDRS);
                    }
                    if (baseRates->hasVRS())
                    {
                        re->setVRS(baseVRS);
                    }
                    if (baseRates->hasFixingRS())
                    {
                        re->setFixingRS(baseFixingRS);
                    }
                    stressedMarketRates.push_back(make_pair("Domestic " + stressedDRSS[i].first, re));
                }
            }
        }

        if (baseRates->hasForeignDRS())
        {
            std::shared_ptr<IHaveMarketRateStresses> stressableForeignDRS =
                std::dynamic_pointer_cast<IHaveMarketRateStresses>(baseForeignDRS);
            if (stressableForeignDRS != 0)
            {
                vector<pair<string, std::shared_ptr<RateSource>>> stressedForeignDRSS =
                    stressableForeignDRS->getMarketRateStresses();
                for (size_t i = 0; i < stressedForeignDRSS.size(); ++i)
                {
                    std::shared_ptr<RatesEnvironment> re =
                        std::make_shared<RatesEnvironment>(baseRates->getAnchorDate());
                    std::shared_ptr<DiscountRateSource> stressForeignDRS =
                        std::dynamic_pointer_cast<DiscountRateSource>(stressedForeignDRSS[i].second);
                    re->setForeignDRS(stressForeignDRS);
                    if (baseRates->hasFRS())
                    {
                        if (frsUsesIRD)
                        {
                            Date anchorDate = baseFRS->getAnchorDate();
                            double spotAtT0 = irdFRS->getSpotAtT0();
                            std::shared_ptr<ForwardCurveFX> stressForeignFRS =
                                std::make_shared<ForwardCurveFX>(anchorDate, spotAtT0, baseDRS,
                                                                 stressForeignDRS);
                            re->setFRS(stressForeignFRS);
                        }
                        else
                        {
                            re->setFRS(baseFRS);
                        }
                    }
                    if (baseRates->hasDRS())
                    {
                        re->setDRS(baseDRS);
                    }
                    if (baseRates->hasVRS())
                    {
                        re->setVRS(baseVRS);
                    }
                    if (baseRates->hasFixingRS())
                    {
                        re->setFixingRS(baseFixingRS);
                    }
                    stressedMarketRates.push_back(make_pair("Foreign " + stressedForeignDRSS[i].first, re));
                }
            }
        }

        if (baseRates->hasVRS())
        {
            std::shared_ptr<IHaveMarketRateStresses> stressableVRS =
                std::dynamic_pointer_cast<IHaveMarketRateStresses>(baseVRS);
            if (stressableVRS != 0)
            {
                vector<pair<string, std::shared_ptr<RateSource>>> stressedVRSS =
                    stressableVRS->getMarketRateStresses();
                for (size_t i = 0; i < stressedVRSS.size(); ++i)
                {
                    std::shared_ptr<RatesEnvironment> re =
                        std::make_shared<RatesEnvironment>(baseRates->getAnchorDate());
                    std::shared_ptr<VolatilityRateSource> stressVRS =
                        std::dynamic_pointer_cast<VolatilityRateSource>(stressedVRSS[i].second);
                    re->setVRS(stressVRS);
                    if (baseRates->hasFRS())
                    {
                        re->setFRS(baseFRS);
                    }
                    if (baseRates->hasDRS())
                    {
                        re->setDRS(baseDRS);
                    }
                    if (baseRates->hasForeignDRS())
                    {
                        re->setForeignDRS(baseForeignDRS);
                    }
                    if (baseRates->hasFixingRS())
                    {
                        re->setFixingRS(baseFixingRS);
                    }
                    stressedMarketRates.push_back(make_pair(stressedVRSS[i].first, re));
                }
            }
        }

        if (baseRates->hasFRS())
        {
            std::shared_ptr<IHaveMarketRateStresses> stressableFRS =
                std::dynamic_pointer_cast<IHaveMarketRateStresses>(baseFRS);
            if (stressableFRS != 0)
            {
                vector<pair<string, std::shared_ptr<RateSource>>> stressedFRSS =
                    stressableFRS->getMarketRateStresses();
                for (size_t i = 0; i < stressedFRSS.size(); ++i)
                {
                    std::shared_ptr<RatesEnvironment> re =
                        std::make_shared<RatesEnvironment>(baseRates->getAnchorDate());
                    std::shared_ptr<ForwardRateSource> stressFRS =
                        std::dynamic_pointer_cast<ForwardRateSource>(stressedFRSS[i].second);
                    re->setFRS(stressFRS);
                    if (baseRates->hasVRS())
                    {
                        re->setVRS(baseVRS);
                    }
                    if (baseRates->hasDRS())
                    {
                        re->setDRS(baseDRS);
                    }
                    if (baseRates->hasForeignDRS())
                    {
                        re->setForeignDRS(baseForeignDRS);
                    }
                    if (baseRates->hasFixingRS())
                    {
                        re->setFixingRS(baseFixingRS);
                    }
                    stressedMarketRates.push_back(make_pair(stressedFRSS[i].first, re));
                }
            }
        }

        return stressedMarketRates;
    }
}