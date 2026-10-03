#include <sjd/rates/Discount/InterestRateConvention.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{
   /*======================================================================================
    InterestRateConvention

    =======================================================================================*/

    InterestRateConvention::InterestRateConvention(boost::shared_ptr<DayCounter> dayCounterInput, 
                                                   Compounding compoundingInput, 
                                                   Frequency frequencyInput)
    {
        dayCounter = dayCounterInput;
        compounding = compoundingInput;
        frequency = frequencyInput;
    }
        
    boost::shared_ptr<DayCounter> InterestRateConvention::getDayCounter()
    {
        return dayCounter;
    }

    Compounding InterestRateConvention::getCompounding()
    {
        return compounding;
    }

    Frequency InterestRateConvention::getFrequency()
    {
        return frequency;
    }

    double InterestRateConvention::dfToRate(double df, Date fromDate, Date toDate)
    {
        double time = dayCounter->yearFraction(fromDate, toDate);
        double accumulationFactor = 1 / df;
        double r = 0;
        switch (compounding) 
        {
          case Simple:
            return (accumulationFactor - 1.0) / time;
          case Compounded:
            return (std::pow(accumulationFactor, 1.0 / (frequency*time)) - 1) * frequency;
          case Continuous:
            return std::log(accumulationFactor) / time;
          default:
            return 0;
        }
    }

    double InterestRateConvention::rateToDF(double rate, Date fromDate, Date toDate)
    {
        double time = dayCounter->yearFraction(fromDate, toDate);
        double accumulationFactor;
        switch (compounding) 
        {
          case Simple:
            accumulationFactor = 1.0 + rate * time;
            break;
          case Compounded:
            accumulationFactor = std::pow(1.0+rate/frequency, frequency*time);
            break;
          case Continuous:
            accumulationFactor = std::exp(rate*time);
            break;
          default: // ignore SimpleThenCompounded
            accumulationFactor = 1;
            break;
        }
        return 1 / accumulationFactor;
    }

}