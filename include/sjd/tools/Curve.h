#ifndef SJD_CURVE_INCLUDED
#define SJD_CURVE_INCLUDED

#include <sjd/maths/maths.h>
#include <ql/time/date.hpp>
#include <boost/shared_ptr.hpp>

namespace sjd 
{
   /*======================================================================================
    Curve: 

    NOTE 1) There are no calendar objects here, all dates are assumed to be good dates
            by the time they hit this class
    =======================================================================================*/
    class Curve
    {
    public:
        Curve();
        
        Curve(QuantLib::Date anchorDate, 
              std::vector<double> observationPoints, 
              std::vector<double> observedRates, 
              ArrayInterpolatorType type = LINEAR,
              bool allowExtrapolation = false);  

        void setParameters(QuantLib::Date anchorDate, 
                           std::vector<double> observationPoints, 
                           std::vector<double> observedRates, 
                           ArrayInterpolatorType type = LINEAR,
                           bool allowExtrapolation = false);
        
        virtual ~Curve() {};

        bool isOK(std::string &errorMessage);

        QuantLib::Date getAnchorDate();
        QuantLib::Date getFinalDate();
        ArrayInterpolatorType getType();
        bool getAllowsExtrapolation();

        std::vector<double> getRates();
        void setRates(std::vector<double> updatedRates);

        bool containsRateOnAnchorDate();
        void addPointOnAnchorDate(double valueToAdd);
        double getRate(double toPoint) const;

        std::vector<double> getObservationPoints() const;


    protected:
        QuantLib::Date anchorDate;
        std::vector<double> observationPoints;
        std::vector<double> observedRates;
        ArrayInterpolatorType type;
        boost::shared_ptr<ArrayInterpolator> interpolator;
        bool allowExtrapolation;
    };

}

#endif