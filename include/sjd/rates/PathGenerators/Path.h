#pragma once

#include <algorithm>
#include <sjd/rates/RatesEnvironment.h>

namespace sjd
{

    /*======================================================================================
    Path
    =======================================================================================*/
    class Path : public Validatable
    {
    public:
        Path() {};

        Path(std::vector<std::shared_ptr<RatesEnvironment>> ratesPath);

        void runChecks() override;

        std::shared_ptr<RatesEnvironment> getRatesEnvironment(size_t i);
        std::vector<QuantLib::Date> getPathDates() const;
        size_t getNumberOfSteps() const;

        std::vector<double> getSpotRates();

    protected:
        std::string validationRoot() const override;

        std::vector<std::shared_ptr<RatesEnvironment>> ratesPath;
        std::vector<QuantLib::Date> pathDates;
    };

}
