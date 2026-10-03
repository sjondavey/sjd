
#ifndef SJD_EUROPEANMONEYNESS_INCLUDED
#define SJD_EUROPEANMONEYNESS_INCLUDED

#include "VolatilitySurfaces.h"


namespace sjd 
{

    /*======================================================================================
   VolatilitySurfaceMoneyness

    European Moneyness Surface
    =======================================================================================*/
    class VolatilitySurfaceMoneyness : public VolatiltiySurfaceRateSource
        // VolSurfaceMoneyness
    {
    public:
        
      VolatilitySurfaceMoneyness(QuantLib::Date anchorDate, 
                         std::vector<QuantLib::Date> observationDates, 
                         std::vector<double> moneyness,
                         QuantLib::Matrix volatilityData,
                         SurfaceInterpolatorType interpolatorType,
                         bool extrapolate = true); // row = moneyness, column = time
        
      VolatilitySurfaceMoneyness(QuantLib::Date anchorDate, 
                         std::vector<QuantLib::Date> observationDates, 
                         std::vector<double> moneyness,
                         std::vector<std::vector<double>> volatilityData,
                         SurfaceInterpolatorType interpolatorType,
                         bool extrapolate = true); // row = moneyness, column = time

        virtual ~VolatilitySurfaceMoneyness() {};

        virtual bool isOK();

        virtual bool isInRange(QuantLib::Date date) const;
        virtual bool isInRange(QuantLib::Date date, double strike, double forward) const;

        // Returns vol for a moneyness of 0
        virtual double getVolatility(QuantLib::Date toDate) const;
        virtual double getVolatility(QuantLib::Date toDate, double strike, double forward) const;

        virtual std::shared_ptr<VolatilityRateSource> parallelBump(double spread);
        virtual std::shared_ptr<VolatilityRateSource> rollForward(QuantLib::Date toDate);

    };
}

#endif