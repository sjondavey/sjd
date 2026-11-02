#pragma once

#include <math.h>
#include <memory>
#include <ql/time/date.hpp>
#include <sjd/maths/maths.h>
#include <sjd/tools/Validation.h>
#include <string>

/*======================================================================================
Overview

Probability of Default and Loss Given Default classes
=======================================================================================*/

namespace sjd
{
    /*======================================================================================
    CVA

     =======================================================================================*/
    class CVA : public Validatable
    {
    public:
        virtual ~CVA() {};

        virtual bool isOK();

        virtual double getLGD(QuantLib::Date d) = 0;
        // return 0 if t2 <= t1.
        virtual double getPD(QuantLib::Date d) = 0;
        virtual double getPD(QuantLib::Date t1, QuantLib::Date t2);
        virtual bool extendsTo(QuantLib::Date date) = 0;
        virtual void setAnchorDate(QuantLib::Date date);

    protected:
        std::string validationRoot() const override;

        QuantLib::Date anchorDate;
        std::string className;
    };

    /*======================================================================================
   Flat PD

   Class for CVA assuming that the conditional PD each year is a constant number. Here we
    model default as a Poisson Process with the PD for a future period T defined by
        F(T) = 1 - exp(-hT)
    where h defines the hazard rate of default (or the conditional PD in an infinitesimal
    time period)
    =======================================================================================*/
    class FlatPD : public CVA
    {
    public:
        // PD is the conditional annual risk of default
        FlatPD(QuantLib::Date anchorDate, double pd, double lgd);
        virtual ~FlatPD() {};

        bool isOK();

        virtual void setLGD(double lgdInput);
        virtual double getLGD(QuantLib::Date d);

        virtual void setPD(double pd);
        virtual double getPD(QuantLib::Date d);
        virtual bool extendsTo(QuantLib::Date date);

    private:
        double pd, lgd, hazardRate;
    };

    /*======================================================================================
   CumulativeDefaultProbability

   Class for CVA assuming that the cumulative default probability is input
    =======================================================================================*/
    class CumulativeDefaultProbability : public CVA
    {
    public:
        // PD is the conditional annual risk of default
        CumulativeDefaultProbability(QuantLib::Date anchorDate,
                                     std::vector<QuantLib::Date> dates,
                                     std::vector<double> cdp,
                                     std::vector<double> lgd);
        virtual ~CumulativeDefaultProbability() {};

        bool isOK();
        virtual double getPD(QuantLib::Date d);
        virtual double getLGD(QuantLib::Date d);

        virtual bool extendsTo(QuantLib::Date date);

    private:
        std::vector<QuantLib::Date> observationDates;
        std::vector<double> cumulativeDefaultProbability;
        std::vector<double> lossGivenDefault;
        std::shared_ptr<ArrayInterpolator> defaultInterpolator;
        std::shared_ptr<ArrayInterpolator> lossInterpolator;
    };
}
