
#include <sjd/contracts/Portfolio/Portfolio.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    Portfolio

    =======================================================================================*/
    void Portfolio::setInputVariables()
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("Portfolio");
    }

    Portfolio::Portfolio(vector<std::shared_ptr<SingleInstrumentContract>> inputs)
    {
        setInputVariables();
        contracts = inputs;
        hasMargin = false;
    }

    Portfolio::Portfolio(vector<std::shared_ptr<SingleInstrumentContract>> inputs,
                         std::shared_ptr<Margin> marginInputs)
    {
        setInputVariables();
        contracts = inputs;
        hasMargin = static_cast<bool>(marginInputs);
        margin = marginInputs;
    }

    Portfolio::Portfolio(vector<std::shared_ptr<Contract>> inputs)
    {
        setInputVariables();
        vector<std::shared_ptr<SingleInstrumentContract>> inputsAsSingleInstruments =
            getSingleInstrumentContracts(inputs);

        if (errorTracking->getHasErrors())
        {
            contracts = vector<std::shared_ptr<SingleInstrumentContract>>(0);
            recordIssue("CONTRACT_EXTRACT_FAILED", "Cannot extract single instruments from input contracts",
                        {});
        }
        else
        {
            contracts = inputsAsSingleInstruments;
        }
        hasMargin = false;
    }

    Portfolio::Portfolio(vector<std::shared_ptr<Contract>> inputs, std::shared_ptr<Margin> marginInput)
    {
        setInputVariables();
        vector<std::shared_ptr<SingleInstrumentContract>> inputsAsSingleInstruments =
            getSingleInstrumentContracts(inputs);

        if (errorTracking->getHasErrors())
        {
            contracts = vector<std::shared_ptr<SingleInstrumentContract>>(0);
            recordIssue("CONTRACT_EXTRACT_FAILED", "Cannot extract single instruments from input contracts",
                        {});
        }
        else
        {
            contracts = inputsAsSingleInstruments;
        }
        hasMargin = static_cast<bool>(marginInput);
        margin = marginInput;
    }

    Portfolio::Portfolio(Portfolio& portfolio)
    {
        setInputVariables();
        contracts = portfolio.contracts;

        if (portfolio.hasMargin)
        {
            hasMargin = true;
            margin = portfolio.margin;
        }
        else
        {
            hasMargin = false;
        }
    }

    std::unique_ptr<Contract> Portfolio::clone()
    {
        return std::make_unique<Portfolio>(*this);
    }

    void Portfolio::validate()
    {
        MultipleInstrumentContract::validate();
    }

    void Portfolio::validate(std::shared_ptr<RatesEnvironment> re)
    {
        return MultipleInstrumentContract::validate(re);
    }

    bool Portfolio::isMargined() const
    {
        return hasMargin;
    }

    std::shared_ptr<Margin> Portfolio::getMargin()
    {
        return margin;
    }

    void Portfolio::setMarginBalance(double balance)
    {
        if (hasMargin)
        {
            margin->setMarginBalance(balance);
        }
    }

    double Portfolio::getMarginBalance() const
    {
        if (margin)
        {
            return margin->getMarginBalance();
        }
        return 0;
    }

    double Portfolio::getMarginCall(std::shared_ptr<RatesEnvironment> re)
    {
        if (margin)
        {
            double mtm = getValue(re);
            return margin->getMarginCall(mtm);
        }
        return 0;
    }

    void Portfolio::updateMarginBalanceWithCall(double marginCall)
    {
        if (margin)
        {
            margin->setMarginBalance(margin->getMarginBalance() + marginCall);
        }
    }

}
