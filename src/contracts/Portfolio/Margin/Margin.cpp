#include <sjd/contracts/Portfolio/Margin/Margin.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    void Margin::setMarginParameters(Date anchorDateInput,
                                     bool useMyThresholdInput,
                                     double myThresholdInput,
                                     bool iRequireInitialMarginInput,
                                     double myRequiredInitialMarginInput,
                                     bool useCptyThresholdInput,
                                     double cptyThresholdInput,
                                     bool cptyRequiresInitialMarginInput,
                                     double cptyRequiredInitialMarginInput,
                                     double minimumTransferAmountInput,
                                     double roundingInput)
    {
        anchorDate = anchorDateInput;
        iRequireInitialMargin = iRequireInitialMarginInput;
        myRequiredInitialMargin = myRequiredInitialMarginInput;
        cptyRequiresInitialMargin = cptyRequiresInitialMarginInput;
        cptyRequiredInitialMargin = cptyRequiredInitialMarginInput;
        useMyThreshold = useMyThresholdInput;
        useCptyThreshold = useCptyThresholdInput;
        myThreshold = myThresholdInput;
        cptyThreshold = cptyThresholdInput;
        mta = minimumTransferAmountInput;
        rounding = roundingInput;
        setMarginBalance(0);
    }

    bool Margin::isOK()
    {
        errorTracking->clear();
        if (iRequireInitialMargin && (myRequiredInitialMargin < 0))
        {
            errorTracking->record("MARGIN_INITIAL_NEGATIVE", "My Required Initial Margin cannot be negative",
                                  {"margin", "initial_margin"});
        }
        if (useMyThreshold && (myThreshold < 0))
        {
            errorTracking->record("MARGIN_THRESHOLD_NEGATIVE", "My Threshold cannot be negative",
                                  {"margin", "threshold"});
        }
        if (useCptyThreshold && (cptyThreshold < 0))
        {
            errorTracking->record("MARGIN_COUNTERPARTY_THRESHOLD_NEGATIVE",
                                  "Counterparty Threshold cannot be negative",
                                  {"margin", "counterparty_threshold"});
        }
        if (cptyRequiresInitialMargin && (cptyRequiredInitialMargin < 0))
        {
            errorTracking->record("MARGIN_COUNTERPARTY_INITIAL_NEGATIVE",
                                  "Counterparty Required Initial Margin cannot be negative",
                                  {"margin", "counterparty_initial_margin"});
        }
        if (mta < 0)
        {
            errorTracking->record("MARGIN_MINIMUM_TRANSFER_NEGATIVE",
                                  "Minimum transfer amount cannot be negative",
                                  {"margin", "minimum_transfer"});
        }
        if (rounding <= 0)
        {
            errorTracking->record("MARGIN_ROUNDING_NOT_POSITIVE", "Rounding must be strictly positive",
                                  {"margin", "rounding"});
        }
        return !hasErrors();
    }

    std::string Margin::validationRoot() const
    {
        return "margin";
    }

    void Margin::setAnchorDate(Date anchorDateInput)
    {
        anchorDate = anchorDateInput;
    }

    Date Margin::getAnchorDate()
    {
        return anchorDate;
    }

    void Margin::setMarginBalance(double balanceInput)
    {
        balance = balanceInput;
    }

    double Margin::getMarginBalance()
    {
        return balance;
    }

    void Margin::updateBalanceForGivenMtM(double mtm)
    {
        setMarginBalance(balance + getMarginCall(mtm));
    }

    void Margin::getFutureMarginBalancesAlongPath(vector<double> mtmPath,
                                                  vector<double>& futureMarginBalances)
    {
        futureMarginBalances.assign(mtmPath.size(), 0.0);
        double initialBalance = balance;
        double marginBalance = balance;
        for (size_t i = 0; i < mtmPath.size(); ++i)
        {
            double marginCall = getMarginCall(mtmPath[i]);
            marginBalance += marginCall;
            futureMarginBalances[i] = marginBalance;
            setMarginBalance(futureMarginBalances[i]);
        }
        setMarginBalance(initialBalance);
    }

    /*======================================================================================
    SimpleMargin

     =======================================================================================*/
    SimpleMargin::SimpleMargin(Date anchorDate,
                               bool useMyThreshold,
                               double myThreshold,
                               bool useCptyThreshold,
                               double cptyThreshold,
                               double minimumTransferAmount,
                               double rounding)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("SimpleMargin"));

        Margin::setMarginParameters(anchorDate, useMyThreshold, myThreshold, false, 0, useCptyThreshold,
                                    cptyThreshold, false, 0, minimumTransferAmount, rounding);
    };

    double SimpleMargin::getMarginCall(double mtm)
    {
        double threshold = 0, marginCall = 0;
        if (mtm >= 0)
        {
            if (useMyThreshold)
            {
                threshold = myThreshold;
                marginCall = simpleBilateralMargin(balance, mtm, threshold);
            }
            else
            {
                marginCall = -balance;
            }
        }
        else if (mtm <
                 0) // -ve MtM implies we must post margin with out cpt using their threshold as benchmark for
        {
            if (useCptyThreshold)
            {
                threshold = cptyThreshold;
                marginCall = simpleBilateralMargin(balance, mtm, threshold);
            }
            else
            {
                marginCall = -balance;
            }
        }
        return marginCall;
    }

    double SimpleMargin::simpleBilateralMargin(double openingBalance, double mtm, double threshold)
    {
        double marginCall = 0;
        if (abs(mtm) > threshold)
        {
            double targetMarginBalace = 0;
            if (mtm < 0)
            {
                targetMarginBalace = mtm + threshold;
            }
            else
            {
                targetMarginBalace = mtm - threshold;
            }
            marginCall = targetMarginBalace - openingBalance;
            if (abs(marginCall) < mta)
            {
                marginCall = 0;
            }
            else
            {
                marginCall = round(marginCall);
            }
        }
        else // Inside the threshold means we must return all the margin
        {
            marginCall = -openingBalance;
        }
        return marginCall;
    }

    double SimpleMargin::round(double unroundedCall)
    {
        double roundedCall = unroundedCall / rounding;
        if (boost::math::sign<double>(unroundedCall) ==
                boost::math::sign<double>(balance) || // Delivering more
            (abs(unroundedCall) > abs(balance)))      // reversing the position
        {                                             // Round away from zero
            if (unroundedCall > 0)
            {
                roundedCall = ceil(roundedCall);
            }
            else
            {
                roundedCall = floor(roundedCall);
            }
        }
        else // Round towards zero
        {
            if (unroundedCall > 0)
            {
                roundedCall = floor(roundedCall);
            }
            else
            {
                roundedCall = ceil(roundedCall);
            }
        }
        roundedCall *= rounding;
        return roundedCall;
    }

}