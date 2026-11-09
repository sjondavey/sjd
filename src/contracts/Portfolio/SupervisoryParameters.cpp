#include <sjd/contracts/Portfolio/SupervisoryParameters.h>

#include <sjd/tools/Errors.h>

#include <cctype>
#include <cstddef>
#include <string>

namespace sjd
{
    namespace
    {

        std::string lower(std::string text)
        {
            for (char& character : text)
            {
                character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
            }
            return text;
        }

        // CEM add-on, then SA-CCR supervisory factor, correlation, option volatility.
        // Gold uses the FX CEM column. CEM has no commodity subclass, so those rows
        // share the other-commodity add-on. Interest rate and FX store correlation 0
        // because SA-CCR does not use one correlation for them. Precious metals have
        // no SA-CCR row; the metals commodity parameters are stored there.
        const SupervisoryFactors kTable[] = {
            {AssetClass::InterestRate, 0.0, 0.005, 0.015, 0.005, 0.0, 0.50},
            {AssetClass::FX, 0.01, 0.05, 0.075, 0.04, 0.0, 0.15},
            {AssetClass::Gold, 0.01, 0.05, 0.075, 0.04, 0.0, 0.15},
            {AssetClass::Equity, 0.06, 0.08, 0.10, 0.32, 0.50, 1.20},
            {AssetClass::PreciousMetal, 0.07, 0.07, 0.08, 0.18, 0.40, 0.70},
            {AssetClass::Commodity, 0.10, 0.12, 0.15, 0.18, 0.40, 0.70},
            {AssetClass::Electricity, 0.10, 0.12, 0.15, 0.40, 0.40, 1.50},
            {AssetClass::OilGas, 0.10, 0.12, 0.15, 0.18, 0.40, 0.70},
            {AssetClass::Metals, 0.10, 0.12, 0.15, 0.18, 0.40, 0.70},
            {AssetClass::Agricultural, 0.10, 0.12, 0.15, 0.18, 0.40, 0.70},
            {AssetClass::OtherCommodity, 0.10, 0.12, 0.15, 0.18, 0.40, 0.70},
        };
        static_assert(sizeof(kTable) / sizeof(kTable[0]) ==
                          static_cast<std::size_t>(AssetClass::OtherCommodity) + 1,
                      "supervisory table must have one row per asset class");

    }

    double SupervisoryFactors::getFactor(double years) const
    {
        if (years <= 1)
            return cemToOneYear;
        if (years <= 5)
            return cemToFiveYears;
        return cemBeyondFiveYears;
    }

    const SupervisoryFactors& supervisoryFactors(AssetClass assetClass)
    {
        return kTable[static_cast<size_t>(assetClass)];
    }

    double cemAddOnFactor(AssetClass assetClass, double years)
    {
        return supervisoryFactors(assetClass).getFactor(years);
    }

    AssetClass assetClassFromName(const std::string& name)
    {
        const std::string key = lower(name);
        if (key == "ir" || key == "interest_rate" || key == "interestrate")
            return AssetClass::InterestRate;
        if (key == "fx")
            return AssetClass::FX;
        if (key == "gold" || key == "xau")
            return AssetClass::Gold;
        if (key == "equity")
            return AssetClass::Equity;
        if (key == "precious" || key == "precious_metal" || key == "preciousmetal")
            return AssetClass::PreciousMetal;
        if (key == "commodity")
            return AssetClass::Commodity;
        if (key == "electricity")
            return AssetClass::Electricity;
        if (key == "oilgas" || key == "oil_gas" || key == "oil/gas")
            return AssetClass::OilGas;
        if (key == "metals")
            return AssetClass::Metals;
        if (key == "agricultural")
            return AssetClass::Agricultural;
        if (key == "other" || key == "other_commodity" || key == "othercommodity")
            return AssetClass::OtherCommodity;
        throw ConfigurationError(
            "asset class must be ir, fx, gold, equity, precious, commodity, electricity, oilgas, metals, "
            "agricultural, or other");
    }

    SupervisoryFactors supervisoryFactors(const std::string& name)
    {
        return supervisoryFactors(assetClassFromName(name));
    }

}
