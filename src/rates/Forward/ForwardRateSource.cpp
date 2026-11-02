#include <sjd/rates/Forward/ForwardRateSource.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{
    vector<double> ForwardRateSource::getForward(vector<Date> toDates) const
    {
        vector<double> rates;
        for (size_t i = 0; i < toDates.size(); ++i)
        {
            rates.push_back(getForward(toDates[i]));
        }
        return rates;
    }

    double ForwardRateSource::getForwardFworwardAdditive(Date fromDate, Date toDate) const
    {
        return getForward(toDate) - getForward(fromDate);
    }

    double ForwardRateSource::getForwardFworwardMultiplicative(Date fromDate, Date toDate) const
    {
        return getForward(toDate) / getForward(fromDate);
    }

    vector<double> ForwardRateSource::getForwardFworwardAdditive(Date fromDate, vector<Date> toDates) const
    {
        vector<double> rates = getForward(toDates);
        double fromRate = getForward(fromDate);
        for (size_t i = 0; i < toDates.size(); ++i)
        {
            rates[i] -= fromRate;
        }
        return rates;
    }

    vector<double> ForwardRateSource::getForwardFworwardMultiplicative(Date fromDate,
                                                                       vector<Date> toDates) const
    {
        vector<double> rates = getForward(toDates);
        double fromRate = getForward(fromDate);
        for (size_t i = 0; i < toDates.size(); ++i)
        {
            rates[i] /= fromRate;
        }
        return rates;
    }

}