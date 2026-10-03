#include <sjd/contracts/ContractStressor.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{
    ContractStressor::ContractStressor(boost::shared_ptr<Contract> contractInput, 
                                       boost::shared_ptr<RatesEnvironment> rates)
    {
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("ContractStressor"));
        contract = contractInput;
        baseRates = rates;
    }

    bool ContractStressor::isOK()
    {
        errorTracking->clear();
        if (!baseRates->isOK())
        {
            errorTracking->absorb(baseRates->validationReport(), {"contract_stressor", "rates_environment"});
            return false;
        }
        contract->validate(baseRates);
        if (!contract->isOK())
        {
            errorTracking->absorb(contract->validationReport(), {"contract_stressor", "contract"});
            return false;
        }
        return true;
    }

    vector<string> ContractStressor::getErrorMessages() const
    {
        return errorTracking->getErrorMessages();
    }

    string ContractStressor::getErrorMessagesAsString() const
    {
        return errorTracking->getErrorMessagesAsString();
    }

    vector<pair<string, double> > ContractStressor::getStressedValues()
    {
        vector<pair<string, boost::shared_ptr<RatesEnvironment>> > namedRatesEnvironments = getAllRatesEnvironments();
        vector<pair<string, double> > stressedValuations;
        for (size_t i = 0; i < namedRatesEnvironments.size(); ++i) 
        {
            double value = contract->getValue(namedRatesEnvironments[i].second);
            stressedValuations.push_back(make_pair(namedRatesEnvironments[i].first, value));
        }
        return stressedValuations;
    }

    vector<pair<string, boost::shared_ptr<RatesEnvironment>> > ContractStressor::getAllRatesEnvironments()
    {
        vector<pair<string, boost::shared_ptr<RatesEnvironment>>> stressedMarketRates;
        stressedMarketRates.push_back(make_pair("Base",baseRates));

        boost::shared_ptr<ForwardRateSource> baseFRS = baseRates->getFRS();
        boost::shared_ptr<DiscountRateSource> baseDRS = baseRates->getDRS();            
        boost::shared_ptr<DiscountRateSource> baseForeignDRS = baseRates->getForeignDRS();
        boost::shared_ptr<VolatilityRateSource> baseVRS = baseRates->getVRS();
        boost::shared_ptr<FixingRateSource> baseFixingRS = baseRates->getFixingRS();

        boost::shared_ptr<sjd::UsesInterestRateDifferential> irdFRS = 
            boost::dynamic_pointer_cast<sjd::UsesInterestRateDifferential>(baseFRS); 
        bool frsUsesIRD = false;
        if (irdFRS != 0 && baseRates->hasForeignDRS() && baseRates->hasDRS())
        {
            frsUsesIRD = true;
        }

        if (baseRates->hasDRS())
        {
            boost::shared_ptr<IHaveMarketRateStresses> stressableDRS = 
                boost::dynamic_pointer_cast<IHaveMarketRateStresses>(baseDRS);
            if (stressableDRS != 0)
            {
                vector<pair<string, boost::shared_ptr<RateSource>>> stressedDRSS = 
                    stressableDRS->getMarketRateStresses();
                for (size_t i = 0; i < stressedDRSS.size(); ++i)
                {
                    boost::shared_ptr<RatesEnvironment> re = boost::shared_ptr<RatesEnvironment>(
                        new RatesEnvironment(baseRates->getAnchorDate()));
                    boost::shared_ptr<DiscountRateSource> stressDRS = 
                        boost::dynamic_pointer_cast<DiscountRateSource>(stressedDRSS[i].second);
                    re->setDRS(stressDRS);
                    if (baseRates->hasFRS())
                    {
                        if (frsUsesIRD)
                        {
                            Date anchorDate = baseFRS->getAnchorDate();
                            double spotAtT0 = irdFRS->getSpotAtT0();
                            boost::shared_ptr<ForwardCurveFX> stressFRS = boost::shared_ptr<ForwardCurveFX>(
                                new ForwardCurveFX(anchorDate, spotAtT0, stressDRS, baseForeignDRS));
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
            boost::shared_ptr<IHaveMarketRateStresses> stressableForeignDRS = 
                boost::dynamic_pointer_cast<IHaveMarketRateStresses>(baseForeignDRS);
            if (stressableForeignDRS != 0)
            {
                vector<pair<string, boost::shared_ptr<RateSource>>> stressedForeignDRSS = 
                    stressableForeignDRS->getMarketRateStresses();
                for (size_t i = 0; i < stressedForeignDRSS.size(); ++i)
                {
                    boost::shared_ptr<RatesEnvironment> re = boost::shared_ptr<RatesEnvironment>(
                        new RatesEnvironment(baseRates->getAnchorDate()));
                    boost::shared_ptr<DiscountRateSource> stressForeignDRS = 
                        boost::dynamic_pointer_cast<DiscountRateSource>(stressedForeignDRSS[i].second);
                    re->setForeignDRS(stressForeignDRS);
                    if (baseRates->hasFRS())
                    {
                        if (frsUsesIRD)
                        {
                            Date anchorDate = baseFRS->getAnchorDate();
                            double spotAtT0 = irdFRS->getSpotAtT0();
                            boost::shared_ptr<ForwardCurveFX> stressForeignFRS = boost::shared_ptr<ForwardCurveFX>(
                                new ForwardCurveFX(anchorDate, spotAtT0, baseDRS, stressForeignDRS));
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
            boost::shared_ptr<IHaveMarketRateStresses> stressableVRS = 
                boost::dynamic_pointer_cast<IHaveMarketRateStresses>(baseVRS);
            if (stressableVRS != 0)
            {
                vector<pair<string, boost::shared_ptr<RateSource>>> stressedVRSS = 
                    stressableVRS->getMarketRateStresses();
                for (size_t i = 0; i < stressedVRSS.size(); ++i)
                {
                    boost::shared_ptr<RatesEnvironment> re = boost::shared_ptr<RatesEnvironment>(
                        new RatesEnvironment(baseRates->getAnchorDate()));
                    boost::shared_ptr<VolatilityRateSource> stressVRS = 
                        boost::dynamic_pointer_cast<VolatilityRateSource>(stressedVRSS[i].second);
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
            boost::shared_ptr<IHaveMarketRateStresses> stressableFRS = 
                boost::dynamic_pointer_cast<IHaveMarketRateStresses>(baseFRS);
            if (stressableFRS != 0)
            {
                vector<pair<string, boost::shared_ptr<RateSource>>> stressedFRSS = 
                    stressableFRS->getMarketRateStresses();
                for (size_t i = 0; i < stressedFRSS.size(); ++i)
                {
                    boost::shared_ptr<RatesEnvironment> re = boost::shared_ptr<RatesEnvironment>(
                        new RatesEnvironment(baseRates->getAnchorDate()));
                    boost::shared_ptr<ForwardRateSource> stressFRS = 
                        boost::dynamic_pointer_cast<ForwardRateSource>(stressedFRSS[i].second);
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