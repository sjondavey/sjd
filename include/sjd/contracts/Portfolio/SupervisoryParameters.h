#pragma once

#include <string>

namespace sjd
{

    // Basel II asset classes, plus the SA-CCR commodity subclasses. CEM reads the
    // add-on column. SA-CCR (not implemented here) reads the other three.
    enum class AssetClass
    {
        InterestRate,
        FX,
        Gold,
        Equity,
        PreciousMetal,
        Commodity,
        Electricity,
        OilGas,
        Metals,
        Agricultural,
        OtherCommodity
    };

    // One row of the supervisory table. CEM add-ons are the Annex 4 percentages.
    // supervisoryFactor, correlation and optionVolatility are the SA-CCR parameters.
    struct SupervisoryFactors
    {
        AssetClass assetClass = AssetClass::Commodity;
        double cemToOneYear = 0;
        double cemToFiveYears = 0;
        double cemBeyondFiveYears = 0;
        double supervisoryFactor = 0;
        double correlation = 0;
        double optionVolatility = 0;

        double getFactor(double years) const;
    };

    struct CounterpartyCredit
    {
        double riskWeight = 1.0;
        double capitalRatio = 0.08;
    };

    const SupervisoryFactors& supervisoryFactors(AssetClass assetClass);
    double cemAddOnFactor(AssetClass assetClass, double years);

    // ir, fx, gold, equity, precious, commodity, electricity, oilgas, metals,
    // agricultural, other. Unknown names throw ConfigurationError.
    AssetClass assetClassFromName(const std::string& name);
    SupervisoryFactors supervisoryFactors(const std::string& name);

}
