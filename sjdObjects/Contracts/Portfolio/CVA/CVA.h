#ifndef SJD_OBJ_CVA
#define SJD_OBJ_CVA

#include <oh/libraryobject.hpp>
#include <sjd/contracts/Portfolio/CVA/cva.h>

namespace sjdObjects 
{

   /*======================================================================================
   CVA

    =======================================================================================*/
    class CVA : public ObjectHandler::LibraryObject<sjd::CVA> 
    {
    public:
        bool extendsTo(Date endDate);
        double getLGD(Date t1);
      // return 0 if t2 <= t1
      double getPD(Date t1, Date t2);
      bool isOK();
      string getErrorMessage();

    protected: 
        OH_LIB_CTOR(CVA, sjd::CVA)
    };

   /*======================================================================================
   FlatPD

    Class for CVA assuming that the conditional PD each year is a constant number.
    =======================================================================================*/
    class FlatPD : public CVA
    {
    public:
        FlatPD(const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
               Date anchorDate, double pd, double lgd,
               bool permanent);
              
    protected:
        OH_OBJ_CTOR(FlatPD, CVA);
    };


    /*======================================================================================
   CumulativeDefaultProbability

   Class for CVA assuming that the cumulative default probability is input
    =======================================================================================*/
    class CumulativeDefaultProbability : public CVA
    {
    public:
        CumulativeDefaultProbability(
                        const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
                        Date anchorDate,    
                        vector<Date> dates, 
                        vector<double> cdp, 
                        vector<double> lgd,
                        bool permanent);

    protected:
        OH_OBJ_CTOR(CumulativeDefaultProbability, CVA);
   };
}

#endif
