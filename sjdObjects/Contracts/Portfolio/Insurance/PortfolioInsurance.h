#ifndef SJD_OBJ_INSURANCE
#define SJD_OBJ_INSURANCE

#include <oh/libraryobject.hpp>
#include <sjd/contracts/Portfolio/Insurance/PortfolioInsurance.h>
#include <sjd/sjdObjects/RateSource/Discount/DiscountRateSource.h>


namespace sjdObjects 
{

    /*======================================================================================
    PortfolioInsurance

    =======================================================================================*

    class PortfolioInsurance : public ObjectHandler::LibraryObject<sjd::PortfolioInsurance> 
    {
    public:
      bool isOK();
      string getErrorMessage();

        double getSumInsured(Date date);
        vector<double> getInsurancePayments(vector<Date> dates, vector<double> mtm);

    protected: 
        OH_LIB_CTOR(PortfolioInsurance, sjd::PortfolioInsurance)
    };



    /*======================================================================================
    SimpleInsuranceWithTermStructure

    =======================================================================================*
    class SimpleInsuranceWithTermStructure : public PortfolioInsurance
    {
    public:
        SimpleInsuranceWithTermStructure(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                              Date policyStartDate, Date policyMaturityDate, 
                                        vector<Date> dates, vector<double> insuredAmounts,
                                        boost::shared_ptr<DiscountRateSource> standbyDRS, 
                                        boost::shared_ptr<DiscountRateSource> utilisationDRS,
                              bool permanent);
              
    protected:
        OH_OBJ_CTOR(SimpleInsuranceWithTermStructure, PortfolioInsurance);
    };
    */
}

#endif
