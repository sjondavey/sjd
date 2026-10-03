#include <sjd/rates/Forward/ForwardCurveFX.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    ForwardCurveFX::ForwardCurveFX(Date anchorDate,
                                   double spot,
                                   Date spotDate,
                                   std::shared_ptr<DiscountRateSource> domesticDRS,
                                   std::shared_ptr<DiscountRateSource> foreignDRS)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("ForwardCurveFX"));
        double spotAtAnchor;
        if (!domesticDRS->extendsTo(spotDate) || (!foreignDRS->extendsTo(spotDate)))
        {
            spotAtAnchor = spot;
        }
        else
        {
            spotAtAnchor = spot * domesticDRS->getDiscountFactor(spotDate) / foreignDRS->getDiscountFactor(spotDate);
        }
        setParameters(anchorDate, spotAtAnchor, domesticDRS, foreignDRS);
    }

    ForwardCurveFX::ForwardCurveFX(Date anchorDate,
                                   double spotAtT0,
                                   std::shared_ptr<DiscountRateSource> domesticDRS,
                                   std::shared_ptr<DiscountRateSource> foreignDRS)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("ForwardCurveFX"));
        setParameters(anchorDate, spotAtT0, domesticDRS, foreignDRS);
    }

    void ForwardCurveFX::setParameters(Date anchorDateInput,
                                       double spotInput,
                                       std::shared_ptr<DiscountRateSource> domesticDRSInput,
                                       std::shared_ptr<DiscountRateSource> foreignDRSInput)
    {
        quotes.setInterestDifferential(anchorDateInput,
                                       spotInput,
                                       domesticDRSInput,
                                       foreignDRSInput,
                                       ForwardRoll::Deterministic,
                                       std::shared_ptr<VolatilityRateSource>());
        anchorDate = quotes.anchorDate();
        allowExtrapolation = quotes.allowsExtrapolation();
        finalDate = quotes.finalDate();
    }

    void ForwardCurveFX::setSpotAtT0(double spotAtT0Input)
    {
        quotes.setSpotAtT0(spotAtT0Input);
    }

    double ForwardCurveFX::getSpotAtT0()
    {
        return quotes.spotAtT0();
    }

    void ForwardCurveFX::setRateSources(std::shared_ptr<DiscountRateSource> domesticDRSInput,
                                        std::shared_ptr<DiscountRateSource> foreignDRSInput)
    {
        quotes.setRateSources(domesticDRSInput, foreignDRSInput);
        allowExtrapolation = quotes.allowsExtrapolation();
        finalDate = quotes.finalDate();
    }

    ForwardCurveFX::~ForwardCurveFX() {}

    bool ForwardCurveFX::isOK()
    {
        if (!ForwardRateSource::isOK())
        {
            return false;
        }
        if (!quotes.domesticRateSource()->isOK())
        {
            absorbChildReport(quotes.domesticRateSource()->validationReport(), "domestic_discount_rate_source");
            return false;
        }
        if (quotes.domesticRateSource()->getAnchorDate() != anchorDate)
        {
            recordIssue("FX_DOMESTIC_ANCHOR_MISMATCH",
                        "Domestic DRS does not have the correct anchor date",
                        {"domestic_discount_rate_source"});
            return false;
        }
        if (!quotes.foreignRateSource()->isOK())
        {
            absorbChildReport(quotes.foreignRateSource()->validationReport(), "foreign_discount_rate_source");
            return false;
        }
        if (quotes.foreignRateSource()->getAnchorDate() != anchorDate)
        {
            recordIssue("FX_FOREIGN_ANCHOR_MISMATCH",
                        "Foreign DRS does not have the correct anchor date",
                        {"foreign_discount_rate_source"});
            return false;
        }
        return !errorTracking->getHasErrors();
    }

    double ForwardCurveFX::getForward(Date toDate) const
    {
        return quotes.getForward(toDate);
    }

}
