#include <sjd/rates/PathGenerators/Path.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd 
{
    Path::Path(vector<boost::shared_ptr<RatesEnvironment>> ratesPathInput)
    {
        errorTracking = boost::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("Path"));
        ratesPath = ratesPathInput;
        pathDates.clear();
        for (size_t i = 0; i < ratesPath.size(); i++)
        {
            pathDates.push_back(ratesPath[i]->getAnchorDate());
        }
    }

    void Path::resetErrorMessages()
    {
        errorTracking->clear();
    }

    bool Path::isOK()
    {
        resetErrorMessages();
        if (ratesPath.size() != pathDates.size())
        {
            errorTracking->record("PATH_LENGTH_MISMATCH", "number of rates does not match the number of dates", {"path"});
            return false;
        }
        for (size_t i = 0; i < ratesPath.size(); i++)
        {
            if (ratesPath[i]->getAnchorDate() != pathDates[i])
            {
                errorTracking->record("PATH_ANCHOR_MISMATCH", "Rates anchor dates do not match path dates", {"path", "rates", std::to_string(i)});
                return false;
            }
            if (!ratesPath[i]->isOK()) 
            {
                errorTracking->absorb(ratesPath[i]->validationReport(), {"path", "rates", std::to_string(i)});
            }
        }
        return !(errorTracking->getHasErrors());
    }

    vector<string> Path::getErrorMessage()
    {
        return errorTracking->getErrorMessages();
    }

    vector<Date> Path::getPathDates() const
    {
        return pathDates;
    }

    boost::shared_ptr<RatesEnvironment> Path::getRatesEnvironment(size_t i)
    {
        return ratesPath[i];
    }

    size_t Path::getNumberOfSteps() const
    {
        return pathDates.size();
    }

    vector<double> Path::getSpotRates()
    {
        vector<double> spot = vector<double>(ratesPath.size());
        vector<boost::shared_ptr<RatesEnvironment>>::iterator it;
        size_t i = 0;
        while (it != ratesPath.end())
        {
            Date d1 = ((*it)->getAnchorDate());
            spot[i] = ((*it)->getFRS())->getForward(d1);
            ++it;
            ++i;
        }
        return spot;
    }
}