#include <sjd/contracts/Portfolio/CVA/cva.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    /*======================================================================================
    CVA

     =======================================================================================*/
    bool CVA::isOK()
    {
        errorTracking->clear();
        return !hasErrors();
    }

    std::string CVA::validationRoot() const
    {
        return "cva";
    }

    void CVA::setAnchorDate(Date date)
    {
        anchorDate = date;
    }

    double CVA::getPD(Date d1, Date d2)
    {
        if ((d2 < d1) || (d1 < anchorDate) || (!extendsTo(d2)))
        {
            return 0;
        }

        double pd = 0;
        if (d1 == anchorDate)
        {
            pd = getPD(d2);
        }
        else
        {
            pd = getPD(d2) - getPD(d1);
        }
        if (pd < 0)
        {
            return 0;
        }
        return pd;
    }

    /*======================================================================================
   Flat PD

    =======================================================================================*/
    FlatPD::FlatPD(Date anchorDateInput, double pdInput, double lgdInput)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("FlatPD"));
        setLGD(lgdInput);
        setPD(pdInput);
        setAnchorDate(anchorDateInput);
    }

    bool FlatPD::isOK()
    {
        CVA::isOK();
        if ((pd < 0) || (pd > 1))
        {
            errorTracking->record("CVA_PD_RANGE", "PD is not in the range [0,1]", {"cva", "pd"});
        }
        if ((lgd < 0) || (lgd > 1))
        {
            errorTracking->record("CVA_LGD_RANGE", "LDG is not in the range [0,1]", {"cva", "lgd"});
        }
        return !errorTracking->getHasErrors();
    }

    void FlatPD::setLGD(double lgdInput)
    {
        lgd = lgdInput;
    }

    double FlatPD::getLGD(Date /*d*/)
    {
        return lgd;
    }

    void FlatPD::setPD(double pdInput)
    {
        pd = pdInput;
        hazardRate = -log(1 - pd);
    }

    double FlatPD::getPD(Date d)
    {
        if (!extendsTo(d))
        {
            return 0;
        }

        if (d == anchorDate)
        {
            return 0;
        }

        double t1 = (d - anchorDate) / 365.0;
        double pd1 = 1 - exp(-t1 * hazardRate);
        return pd1;
    }

    bool FlatPD::extendsTo(Date date)
    {
        return (date >= anchorDate);
    }

    /*======================================================================================
   CumulativeDefaultProbability

    =======================================================================================*/
    CumulativeDefaultProbability::CumulativeDefaultProbability(Date anchorDateInput,
                                                               vector<Date> datesInput,
                                                               vector<double> cdpInput,
                                                               vector<double> lgdInput)
    {
        errorTracking =
            std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("CumulativeDefaultProbability"));
        setAnchorDate(anchorDateInput);
        observationDates = datesInput;
        vector<double> cdp = cdpInput;
        vector<double> lgd = lgdInput;

        if (observationDates[0] > anchorDate)
        {
            observationDates.insert(observationDates.begin(), anchorDate);
            cdp.insert(cdp.begin(), 0);
            lgd.insert(lgd.begin(), 0);
        }

        cumulativeDefaultProbability = cdp;
        lossGivenDefault = lgd;
        vector<double> time;
        for (size_t i = 0; i < observationDates.size(); ++i)
        {
            time.push_back((observationDates[i] - anchorDate) / 365.0);
        }
        defaultInterpolator =
            std::shared_ptr<ArrayInterpolator>(new CubicSplineInterpolator(time, cdp, false));
        lossInterpolator = std::shared_ptr<ArrayInterpolator>(new CubicSplineInterpolator(time, lgd, false));
    }

    bool CumulativeDefaultProbability::isOK()
    {
        CVA::isOK();
        if (observationDates[0] < anchorDate)
        {
            errorTracking->record("CVA_DATES_BEFORE_ANCHOR", "Observation dates before anchor date",
                                  {"cva", "observation_dates"});
        }
        if (!isStrictlyIncreasing<double>(cumulativeDefaultProbability))
        {
            errorTracking->record("CVA_PD_NOT_INCREASING",
                                  "Cumulative default probabilities are not strictly increasing",
                                  {"cva", "cumulative_pd"});
        }
        if ((cumulativeDefaultProbability.back() > 1) || (cumulativeDefaultProbability.front() < 0))
        {
            errorTracking->record("CVA_PD_RANGE", "Cumulative default probability not in the range [0, 1]",
                                  {"cva", "cumulative_pd"});
        }
        if (abs(cumulativeDefaultProbability.front()) < -1e-14)
        {
            errorTracking->record("CVA_PD_ANCHOR_NOT_ZERO",
                                  "Cumulative default probability on the anchor date is not 0",
                                  {"cva", "cumulative_pd"});
        }
        if (!defaultInterpolator->isOk())
        {
            errorTracking->record("CVA_PD_INTERPOLATOR", defaultInterpolator->getErrorMessage(),
                                  {"cva", "cumulative_pd"});
        }
        for (size_t i = 0; i < lossGivenDefault.size(); ++i)
        {
            if ((lossGivenDefault[i] < 0) || (lossGivenDefault[i] > 1))
            {
                errorTracking->record("CVA_LGD_RANGE", "Loss given default must be between 0 and 1",
                                      {"cva", "lgd"});
            }
        }
        if (!lossInterpolator->isOk())
        {
            errorTracking->record("CVA_LGD_INTERPOLATOR", lossInterpolator->getErrorMessage(),
                                  {"cva", "lgd"});
        }
        return !errorTracking->getHasErrors();
    }

    double CumulativeDefaultProbability::getPD(Date d)
    {
        if (!extendsTo(d))
        {
            return 0;
        }
        double t1 = (d - anchorDate) / 365.0;
        double cpd1 = defaultInterpolator->getRate(t1);
        return cpd1;
    }

    double CumulativeDefaultProbability::getLGD(Date d)
    {
        if (!extendsTo(d))
        {
            return 0;
        }
        double t1 = (d - anchorDate) / 365.0;
        double lgd1 = lossInterpolator->getRate(t1);
        return lgd1;
    }

    bool CumulativeDefaultProbability::extendsTo(Date date)
    {
        return ((date >= anchorDate) && (date <= observationDates.back()));
    }

}
