

#include <ql/auto_link.hpp>

#include <oh/objecthandler.hpp>
#include <ohxl/convert_oper.hpp>
#include <qlo/Conversions/coercehandle.hpp>
#include <qlo/Conversions/coerceobject.hpp>
#include <qlo/Conversions/varianttoquotehandle.hpp>
#include <qlo/Conversions/varianttodate.hpp>
#include <qlo/Conversions/varianttoquote.hpp>
#include <qlo/Conversions/varianttoperiod.hpp>
#include <qlo/Conversions/varianttosize.hpp>
#include <qlo/Conversions/varianttotimeseries.hpp>
#include <qlo/Conversions/conversion_tmpl.hpp>


#include <qlo/qladdindefines.hpp>
#include <oh/enumerations/typefactory.hpp>
#include <qlo/enumerations/factories/calendarfactory.hpp>
#include <qlo/handleimpl.hpp>
#include <qlo/conversions/coercetermstructure.hpp>
#include <qlo/ratehelpers.hpp>
#include <qlo/valueobjects/vo_termstructures.hpp>
#include <qlo/loop/loop_termstructures.hpp>
#include <ohxl/loop.hpp>

#include <ohxl/objecthandlerxl.hpp>
#include <ohxl/callingrange.hpp>
#include <qlxl/session.hpp>
#include <qlxl/conversions/all.hpp>

#include <ql/time/date.hpp>
#include <sjd/Volatility/VolatilityCurve.h>
#include <ql/time/daycounter.hpp>
#include <ql/time/daycounters/all.hpp>
#include <sjd/sjdXllObjects/VolatilityCurveValueObject.h>
#include <sjd/sjdXllObjects/VolatilityCurveObject.h>
#include <oh/ohdefines.hpp>
#include <ohxl/conversions/all.hpp>

namespace ObjectHandler {

    template<>
    boost::shared_ptr<QuantLib::Quote> convert2<boost::shared_ptr<QuantLib::Quote>, ConvertOper>(const ConvertOper& c) {
        return convertQuote(c);
    }

    template<> 
    QuantLib::Date convert2<QuantLib::Date, ConvertOper>(const ConvertOper& c) {
        return convertDate(c);
    }

    template<> 
    QuantLib::Period convert2<QuantLib::Period, ConvertOper>(const ConvertOper& c) {
        return convertPeriod(c);
    }
    
    template<> 
    QuantLib::Size convert2<QuantLib::Size, ConvertOper>(const ConvertOper& p) {
        return convertSize(p); 
    }

    template<>
    QuantLib::Handle<QuantLib::Quote> convert2<QuantLib::Handle<QuantLib::Quote>, ConvertOper>(const ConvertOper& c) {
        return convertQuoteHandle(c);
    }

    template<> 
    QuantLib::TimeSeriesDef convert2<QuantLib::TimeSeriesDef, ConvertOper>(const ConvertOper& c) {
        return convertTimeSeriesDef(c);
    }

}
