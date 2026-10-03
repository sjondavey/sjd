#ifndef MARGIN_INCLUDED
#define MARGIN_INCLUDED
//#pragma once

#include <boost/shared_ptr.hpp>
#include <sjd/tools/Validation.h>
#include <sjd/maths/maths.h>
#include <string>
#include <boost/math/special_functions.hpp> // round, sign

#include <ql/time/date.hpp>
#include <ql/time/calendars/all.hpp>

/*======================================================================================
Overview
Rules on how to calcuate margin calls for given mark-to-markets
=======================================================================================*/

namespace sjd {
   /*======================================================================================
   Margin

    =======================================================================================*/
    class Margin : public Validatable
    {
    public:
        virtual ~Margin() {};

        virtual void setMarginParameters(QuantLib::Date anchorDate,
                                         bool useMyThreshold, 
                                         double myThreshold,
                                         bool iRequireInitialMargin, 
                                         double myRequiredInitialMargin,
                                         bool useCptyThreshold, 
                                         double cptyThreshold,
                                         bool cptyRequiresInitialMargin, 
                                         double cptyRequiredInitialMargin,
                                         double minimumTransferAmount, 
                                         double rounding);
        
        virtual bool isOK();

        void setAnchorDate(QuantLib::Date anchorDate);
        QuantLib::Date getAnchorDate();

        virtual void setMarginBalance(double balanceInput);
        virtual double getMarginBalance();
        virtual void updateBalanceForGivenMtM(double mtm);
        

        //virtual bool setFutureMarginDates(std::vector<QuantLib::Date> dates);

        // futureMarginBalances is resized to match mtmPath. By default this will call for the
        // margin and immediately update the balance accordingly
        virtual void getFutureMarginBalancesAlongPath(std::vector<double> mtmPath, std::vector<double> &futureMarginBalances);

        virtual double getMarginCall(double mtm) = 0;

    protected:
        std::string validationRoot() const override;

        bool useMyThreshold; 
        bool useCptyThreshold;
        bool iRequireInitialMargin;
        bool cptyRequiresInitialMargin;

        double myThreshold;
        double cptyThreshold;
        double mta;
        double rounding;
        double myRequiredInitialMargin;
        double cptyRequiredInitialMargin;
        double balance;
        
        QuantLib::Date anchorDate;
    
        std::string className;
    };


    /*======================================================================================
    SimpleMargin: This class is supposed to represent a simple interbank CSA. Margin movements
    are a function of variation in the MtM (i.e. no initial margin, independent amounts etc). 
    
    According to the 2013 ISDA Standard Credit Support Annex (Transfer - English Law) document, 
    there can be different rounding rules for the Delivery and Return amounts, which are 
    defined as follows:
    - �VM Delivery Amount�: max(0, MtM � VM Credit Support Balance)
    - �VM Return Amount�: max(0, VM Credit Support Balance  - MtM)
    
    Typically the Delivery Amount is rounded *up* and the Return Amount is rounded *down* to 
    the nearest multiple of the rounding amount. 
    =======================================================================================*/
    class SimpleMargin : public Margin
    {
    public:
        SimpleMargin(QuantLib::Date anchorDate,
                     bool useMyThreshold, double myThreshold,
                     bool useCptyThreshold, double cptyThreshold,
                     double minimumTransferAmount,
                     double rounding);

        virtual ~SimpleMargin() {};
        virtual double getMarginCall(double mtm);
    private:
        double simpleBilateralMargin(double openingBalance, double mtm, double threshold);
        double round(double r); 
    };

}

#endif

