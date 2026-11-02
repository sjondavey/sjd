#include <sjd/contracts/Portfolio/RegCap/RegCapCreditParameters.h>

using namespace std;

namespace sjd
{

    RegCapCreditParameters::RegCapCreditParameters(double counterpartyWeighting, double capitalCharge)
    {
        cpw = counterpartyWeighting;
        cc = capitalCharge;
    }

    double RegCapCreditParameters::getCounterpartyWeighting()
    {
        return cpw;
    }

    double RegCapCreditParameters::getCapitalCharge()
    {
        return cc;
    }

}
