#include <sjd/rates/Volatility/VolatilityRateSource.h>

using namespace std;
using namespace QuantLib;

namespace sjd 
{
   /*======================================================================================
   VolatlityRateSource

    =======================================================================================*/
    bool VolatilityRateSource::isOK()
    {
        RateSource::isOK();
        if (dayCounter == NULL) 
        {
            recordIssue("VOLATILITY_DAY_COUNTER_MISSING",
                        "Day counter not set",
                        {"day_counter"});
        }
        return !errorTracking->getHasErrors();
    }

    boost::shared_ptr<QuantLib::DayCounter> VolatilityRateSource::getDayCounter() const 
    {
        return dayCounter;
    }

    bool VolatilityRateSource::hasNegativeATMFwdFwdVariances(vector<Date> checkDates, Date &startOfNegVar, Date &endOfNegVar)
    {
        Date d2 = checkDates[0];
        double sd2 = getStandardDeviation(d2);
        Date d1;
        double sd1;
        for (size_t i = 1; i < checkDates.size(); ++i)
        {
            d1 = d2;
            sd1 = sd2;
            d2 = checkDates[i];
            sd2 = getStandardDeviation(d2);
            double fwdFwdVar = sd2 * sd2 - sd1 * sd1;
            if (fwdFwdVar < 0)
            {
                startOfNegVar = d1;
                endOfNegVar = d2;
                return true;
            }
        }
        return false;
    }

    bool VolatilityRateSource::hasNegativeATMFwdFwdVariances(Date &startOfNegVar, Date &endOfNegVar)
    {
        vector<Date> observationDates = getObservationDates();
        return hasNegativeATMFwdFwdVariances(observationDates, startOfNegVar, endOfNegVar);
    }

    double VolatilityRateSource::getStandardDeviation(Date toDate) const
    {
        double vol = getVolatility(toDate);
        double dt = dayCounter->yearFraction(anchorDate, toDate);
        return vol * sqrt(dt);
    }

    double VolatilityRateSource::getStandardDeviation(Date toDate, double strike, double forward) const
    {
        double vol = getVolatility(toDate, strike, forward);
        double dt = dayCounter->yearFraction(anchorDate, toDate);
        return vol * sqrt(dt);
    }

   void VolatilityRateSource::getVolAndSD(Date toDate, double &vol, double& sd) const
   {
      vol = getVolatility(toDate);
      sd = getStandardDeviation(toDate);
   }

   void VolatilityRateSource::getVolAndSD(Date toDate, double strike, double forward, double &vol, double& sd) const
   {
      vol = getVolatility(toDate, strike, forward);
      sd = getStandardDeviation(toDate, strike, forward);
   }

   void VolatilityRateSource::getVolAndSD(vector<Date> toDates, 
                                          double strike, 
                                          vector<double> forwards,
                                          vector<double> &vols,
                                          vector<double> &sds) const
   {
       for (size_t i = 0; i < toDates.size(); ++i) 
       {
           getVolAndSD(toDates[i], strike, forwards[i], vols[i], sds[i]);
       }
   }
}