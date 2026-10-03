
#ifndef PATH_INCLUDED
#define Path_INCLUDED

#include <algorithm>
#include <sjd/rates/RatesEnvironment.h>

namespace sjd 
{

    /*======================================================================================
    Path
    =======================================================================================*/
    class Path
    {
    public:
        Path() {};

        Path(std::vector<boost::shared_ptr<RatesEnvironment>> ratesPath);

        void resetErrorMessages();
        bool isOK();
        std::vector<std::string> getErrorMessage();

        boost::shared_ptr<RatesEnvironment> getRatesEnvironment(size_t i);  
        std::vector<QuantLib::Date>  getPathDates() const;
        size_t getNumberOfSteps() const;

        std::vector<double> getSpotRates();

    protected:

        boost::shared_ptr<sjd::IssueTracker> errorTracking;

        std::vector<boost::shared_ptr<RatesEnvironment>> ratesPath;
        std::vector<QuantLib::Date> pathDates;
    };


}

#endif