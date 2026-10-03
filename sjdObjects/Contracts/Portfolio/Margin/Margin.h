#ifndef SJD_OBJ_MARGIN
#define SJD_OBJ_MARGIN

#include <oh/libraryobject.hpp>
#include <sjd/contracts/Portfolio/Margin/Margin.h>

namespace sjdObjects 
{

   /*======================================================================================
   Margin

    =======================================================================================*/
    class Margin : public ObjectHandler::LibraryObject<sjd::Margin> 
    {
    public:
      virtual bool isOK();
      virtual string getErrorMessage();

        void setMarginBalance(double balance);
        double getMarginBalance();
        double getMarginCall(double mtm);

    protected: 
        OH_LIB_CTOR(Margin, sjd::Margin)
    };



    /*======================================================================================
    SimpleMargin

    =======================================================================================*/
    class SimpleMargin : public Margin
    {
    public:
        SimpleMargin(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                     Date anchorDate, 
                     bool useMyThreshold, 
                     double myThreshold,
                     bool useCptyThreshold, 
                     double cptyThreshold,
                     double minimumTransferAmount, 
                     double rounding,
                     bool permanent);
              
    protected:
        OH_OBJ_CTOR(SimpleMargin, Margin);
    };
}

#endif
