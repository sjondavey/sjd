#include <sjd/rates/Volatility/VolatilityCurve.h>

#include <boost/lexical_cast.hpp> // convert numbers to string

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{

    /*======================================================================================
    VolatilityCurve 
    =======================================================================================*/
    VolatilityCurve::VolatilityCurve(Date anchorDateInput, 
                                    vector<Date> observationDatesInput, 
                                    vector<double> volatilityInput, 
                                    boost::shared_ptr<DayCounter> dayCounterInput,
                                    ArrayInterpolatorType typeInput,
                                    bool allowExtrapolationInput) 
    {
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("VolatilityCurve"));

        datedCurve = boost::shared_ptr<DatedCurve>(new DatedCurve(anchorDateInput, 
                                                                  observationDatesInput, 
                                                                  volatilityInput,
                                                                  typeInput,
                                                                  allowExtrapolationInput));

        anchorDate = datedCurve->getAnchorDate();
        allowExtrapolation = datedCurve->getAllowsExtrapolation();
        finalDate = datedCurve->getFinalDate();
        type = typeInput;
        string tmp;
        if (datedCurve->isOK(tmp) && 
           (!datedCurve->containsRateOnAnchorDate()))
        {
            datedCurve->addPointOnAnchorDate(volatilityInput[0]);
        }
        dayCounter = dayCounterInput;
    }

    VolatilityCurve::~VolatilityCurve()
    {}

    bool VolatilityCurve::isOK()
    {
        DeterministicVolatilityRateSource::isOK();
        string errorMessage = "";
        if (!datedCurve->isOK(errorMessage))
        {
            const std::string code = errorMessage.find("same dimension") != std::string::npos
                ? "VOLATILITY_CURVE_DIMENSION_MISMATCH"
                : "VOLATILITY_CURVE_INVALID";
            recordIssue(code, errorMessage, {"inputs"});
            return false;
        }
        for (size_t i = 0; i < datedCurve->getObservationDates().size(); ++i) 
        {
            bool hasNegativeInputs = false;
            if ((!hasNegativeInputs) && 
                (datedCurve->getRates()[i] <= 0) )
            {
                recordIssue("VOLATILITY_CURVE_NEGATIVE_INPUT",
                            "has negative inputs",
                            {"volatilities"});
                hasNegativeInputs = true;
            }
        }
        return !errorTracking->getHasErrors();
    }

    vector<Date> VolatilityCurve::getObservationDates()
    {
        return datedCurve->getObservationDates();
    }

    bool VolatilityCurve::isInRange(Date date) const
    {
      return RateSource::isInRange(date);
    }

    bool VolatilityCurve::isInRange(Date date, double strike, double forward) const
    {
      return RateSource::isInRange(date);
    }

    double VolatilityCurve::getVolatility(Date toDate) const
    {
        return datedCurve->getRate(toDate);
    }

    double VolatilityCurve::getVolatility(Date toDate, double strike, double forward) const         
    {
      return getVolatility(toDate);
    }

    double VolatilityCurve::getStandardDeviation(Date toDate) const 
    {
        double yearFraction = dayCounter->yearFraction(anchorDate, toDate);
        return datedCurve->getRate(toDate) * sqrt(yearFraction);
    }

    double VolatilityCurve::getStandardDeviation(Date toDate, double strike, double forward) const
    {
        return getStandardDeviation(toDate);
    }

    void VolatilityCurve::getVolAndSD(Date toDate, double &vol, double& sd) const
    {
        vol = datedCurve->getRate(toDate);
        double yearFraction = dayCounter->yearFraction(anchorDate, toDate);
        sd = vol * sqrt(yearFraction);
        return;
   }

    void VolatilityCurve::getVolAndSD(Date toDate, double strike, double forward, double &vol, double& sd) const 
    {
        return getVolAndSD(toDate, vol, sd);
    };

    vector<Date> VolatilityCurve::getObservationDates() const
    {
         return datedCurve->getObservationDates();
    }
    
    ArrayInterpolatorType VolatilityCurve::getType()
    {
        return type;
    }

    boost::shared_ptr<VolatilityRateSource> VolatilityCurve::parallelBump(double spread)
    {
        vector<Date> rolledObservationDates;
        vector<double> rolledVols; 
        for (size_t i = 0; i < getObservationDates().size(); ++i)
        {
            rolledObservationDates.push_back(getObservationDates()[i]);
            rolledVols.push_back(getVolatility(getObservationDates()[i])+spread);
        }

       return boost::shared_ptr<VolatilityCurve>(new VolatilityCurve(anchorDate, 
                                                                      rolledObservationDates, 
                                                                      rolledVols, 
                                                                      dayCounter,
                                                                      datedCurve->getType(),
                                                                      allowExtrapolation));     
     }

    boost::shared_ptr<VolatilityRateSource> VolatilityCurve::rollForward(Date toDate)
    {
        vector<Date> rolledObservationDates;
        vector<double> rolledVols; 
        if (!isInRange(toDate))
        {
            rolledObservationDates.push_back(toDate);
            rolledVols.push_back(1.0);
        }
        else
        {
            for (size_t i = 0; i < getObservationDates().size(); ++i)
            {
                if (getObservationDates()[i] >= toDate) 
                {
                    rolledObservationDates.push_back(getObservationDates()[i]);
                    rolledVols.push_back(getVolatility(getObservationDates()[i]));
                }
            }
        }
        return boost::shared_ptr<VolatilityCurve>(new VolatilityCurve(toDate, 
                                                                      rolledObservationDates, 
                                                                      rolledVols, 
                                                                      dayCounter,
                                                                      datedCurve->getType(),
                                                                      allowExtrapolation));     
     }
}