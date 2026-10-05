
#include <sjd/contracts/Portfolio/Portfolio.h>


using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{

    /*======================================================================================
    Portfolio
    
    =======================================================================================*/
    void Portfolio::setInputVariables()
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("Portfolio"));
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
        hasMargin = true;
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
            recordIssue("CONTRACT_EXTRACT_FAILED",
                        "Cannot extract single instruments from input contracts",
                        {});
        }
        else 
        {
            contracts = inputsAsSingleInstruments;
        }
        hasMargin = false;
    }

    Portfolio::Portfolio(vector<std::shared_ptr<Contract>> inputs,
                         std::shared_ptr<Margin> marginInput)
    {
        setInputVariables();
        vector<std::shared_ptr<SingleInstrumentContract>> inputsAsSingleInstruments = 
            getSingleInstrumentContracts(inputs);

        if (errorTracking->getHasErrors()) 
        {
            contracts = vector<std::shared_ptr<SingleInstrumentContract>>(0);
            recordIssue("CONTRACT_EXTRACT_FAILED",
                        "Cannot extract single instruments from input contracts",
                        {});
        }
        else 
        {
            contracts = inputsAsSingleInstruments;
        }
        hasMargin = true;
        margin = marginInput;
    }

    Portfolio::Portfolio(Portfolio &portfolio)
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
        return std::unique_ptr<Contract>(new Portfolio(*this));
    }

    void Portfolio::validate() 
   {
        MultipleInstrumentContract::validate();
    }

    void Portfolio::validate(std::shared_ptr<RatesEnvironment> re) 
    {
        return MultipleInstrumentContract::validate(re);
    }

    bool Portfolio::isMargined()
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

    double Portfolio::getMarginBalance() 
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
