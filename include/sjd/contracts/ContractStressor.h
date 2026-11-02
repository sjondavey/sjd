#ifndef CONTRACTSTRESSOR_INCLUDED
#define CONTRACTSTRESSOR_INCLUDED

#include <algorithm>
#include <list>

#include "Contracts.h"
#include <sjd/rates/Forward/ForwardCurveFX.h>
#include <sjd/rates/Forward/UsesInterestRateDifferential.h>
#include <sjd/rates/IHaveMarketRateStresses.h>
#include <sjd/rates/RatesEnvironment.h>

namespace sjd
{
    class ContractStressor : public Validatable
    {
    public:
        // Use this for unmargined portfolios or portfolios that do not require Holding QuantLib::Period
        // calculations
        ContractStressor(std::shared_ptr<Contract> contract, std::shared_ptr<RatesEnvironment> rates);

        bool isOK();

        std::vector<std::pair<std::string, double>> getStressedValues();

    protected:
        std::vector<std::pair<std::string, std::shared_ptr<RatesEnvironment>>> getAllRatesEnvironments();
        std::string validationRoot() const override;

        std::shared_ptr<Contract> contract;
        std::shared_ptr<RatesEnvironment> baseRates;
    };
}

#endif
