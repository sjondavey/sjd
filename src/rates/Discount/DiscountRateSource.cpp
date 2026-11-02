#include <sjd/rates/Discount/DiscountRateSource.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    double DiscountRateSource::getDiscountFactor(Date fromDate, Date toDate) const
    {
        double fromDF = getDiscountFactor(fromDate);
        double toDF = getDiscountFactor(toDate);
        return toDF / fromDF;
    }

    vector<double> DiscountRateSource::getDiscountFactor(vector<Date> times) const
    {
        vector<double> dfs;
        for (size_t i = 0; i < times.size(); ++i)
        {
            double df = getDiscountFactor(times[i]);
            dfs.push_back(df);
        }
        return dfs;
    }

    vector<double> DiscountRateSource::getDiscountFactor(Date fromTime, vector<Date> times) const
    {
        double fromDF = getDiscountFactor(fromTime);
        vector<double> dfs;
        for (size_t i = 0; i < times.size(); ++i)
        {
            double df = getDiscountFactor(times[i]);
            dfs.push_back(df / fromDF);
        }
        return dfs;
    }
}