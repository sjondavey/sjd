#ifndef CONTRACTSTRESSOR_INCLUDED
#define CONTRACTSTRESSOR_INCLUDED

#include <list>
#include <algorithm>

#include "Contracts.h"
#include <sjd/rates/IHaveMarketRateStresses.h>
#include <sjd/rates/RatesEnvironment.h>
#include <sjd/rates/Forward/UsesInterestRateDifferential.h>
#include <sjd/rates/Forward/ForwardCurveFX.h>

namespace sjd 
{
    class ContractStressor : public Validatable
    {
    public:
        // Use this for unmargined portfolios or portfolios that do not require Holding QuantLib::Period calculations
        ContractStressor(boost::shared_ptr<Contract> contract, 
                         boost::shared_ptr<RatesEnvironment> rates);

        bool isOK();

        std::vector<std::pair<std::string, double> > getStressedValues();

    protected:
        std::vector<std::pair<std::string, boost::shared_ptr<RatesEnvironment>> > getAllRatesEnvironments();
        std::string validationRoot() const override;

        boost::shared_ptr<Contract> contract;
        boost::shared_ptr<RatesEnvironment> baseRates;
    };
}

#endif
