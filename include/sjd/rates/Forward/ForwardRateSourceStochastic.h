#ifndef SJD_STOCHASTICFORWARDRATESOURCE_INCLUDED
#define SJD_STOCHASTICFORWARDRATESOURCE_INCLUDED

#include <sjd/rates/Forward/ForwardRateSource.h>
#include <sjd/rates/Volatility/VolatilityRateSource.h>

#include <boost/enable_shared_from_this.hpp>

namespace sjd 
{

    /*======================================================================================
    ForwardRateSourceStochastic 

    A stochastic forward rate source is a rate source which allows a user to roll the curve 
    to a future date with *one* source of randomness
    =======================================================================================*/
    class ForwardRateSourceStochastic : public ForwardRateSource
    {
    public:
        ForwardRateSourceStochastic() {};

        virtual bool isOK();

        void setVolitalityRateSource(boost::shared_ptr<VolatilityRateSource> vrs);


        virtual boost::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate, double normalRV) = 0;
        // rateSetDates are the sorted dates between the anchorDate and the "toDate" for
        // which we may require rate sets
        virtual boost::shared_ptr<ForwardRateSource> rollForward(QuantLib::Date toDate, 
                                                                 double normalRV,
                                                                 std::vector<QuantLib::Date> rateSetDates,
                                                                 std::vector<double> &rateSetsFromAnchorToDate);


        virtual void setForwardValuationDates(std::set<QuantLib::Date> dates); 
        virtual void setRandomVariables(std::vector<double> normalRVs); 
        virtual std::vector<boost::shared_ptr<ForwardRateSource> > rollForward() = 0;
        virtual std::vector<boost::shared_ptr<ForwardRateSource> > rollForward(std::vector<QuantLib::Date> rateSetDates,
                                                              std::vector<double> &rateSetsFromAnchorToFinalRollDate) = 0;

        // default implementation for independant moves
        virtual std::vector<double> getImpliedRandomNumbers(std::vector<double> spot);

    protected:

        std::vector<double> normalRVs;
        std::vector<QuantLib::Date> forwardSimulationDates;
        boost::shared_ptr<VolatilityRateSource> vrs;

    };

}

#endif