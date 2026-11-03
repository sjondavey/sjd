#include <sjd/rates/Forward/ForwardCurveFX.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    ForwardCurveFX::ForwardCurveFX(Date anchorDate,
                                   double spot,
                                   Date spotDate,
                                   std::shared_ptr<DiscountRateSource> domesticDRS,
                                   std::shared_ptr<DiscountRateSource> foreignDRS)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("ForwardCurveFX");
        double spotAtAnchor;
        if (!domesticDRS->extendsTo(spotDate) || (!foreignDRS->extendsTo(spotDate)))
        {
            spotAtAnchor = spot;
        }
        else
        {
            spotAtAnchor =
                spot * domesticDRS->getDiscountFactor(spotDate) / foreignDRS->getDiscountFactor(spotDate);
        }
        setParameters(anchorDate, spotAtAnchor, domesticDRS, foreignDRS);
    }

    ForwardCurveFX::ForwardCurveFX(Date anchorDate,
                                   double spotAtT0,
                                   std::shared_ptr<DiscountRateSource> domesticDRS,
                                   std::shared_ptr<DiscountRateSource> foreignDRS)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("ForwardCurveFX");
        setParameters(anchorDate, spotAtT0, domesticDRS, foreignDRS);
    }

    void ForwardCurveFX::setParameters(Date anchorDateInput,
                                       double spotInput,
                                       std::shared_ptr<DiscountRateSource> domesticDRSInput,
                                       std::shared_ptr<DiscountRateSource> foreignDRSInput)
    {
        quotes.setInterestDifferential(anchorDateInput, spotInput, domesticDRSInput, foreignDRSInput,
                                       ForwardRoll::Deterministic, std::shared_ptr<VolatilityRateSource>());
        anchorDate = quotes.anchorDate();
        allowExtrapolation = quotes.allowsExtrapolation();
        flatBackFilled = quotes.anchorWasFlatFilled();
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
        flatBackFilled = quotes.anchorWasFlatFilled();
        finalDate = quotes.finalDate();
    }

    ForwardCurveFX::~ForwardCurveFX() {}

    void ForwardCurveFX::runChecks()
    {
        ForwardRateSource::runChecks();
        if (hasErrors())
        {
            return;
        }
        if (!quotes.domesticRateSource()->validate().isOK())
        {
            absorbChildReport(quotes.domesticRateSource()->validationReport(),
                              "domestic_discount_rate_source");
            return;
        }
        if (quotes.domesticRateSource()->getAnchorDate() != anchorDate)
        {
            recordIssue(issues::FX_DOMESTIC_ANCHOR_MISMATCH,
                        "Domestic DRS does not have the correct anchor date",
                        {"domestic_discount_rate_source"});
            return;
        }
        if (!quotes.foreignRateSource()->validate().isOK())
        {
            absorbChildReport(quotes.foreignRateSource()->validationReport(), "foreign_discount_rate_source");
            return;
        }
        if (quotes.foreignRateSource()->getAnchorDate() != anchorDate)
        {
            recordIssue(issues::FX_FOREIGN_ANCHOR_MISMATCH,
                        "Foreign DRS does not have the correct anchor date",
                        {"foreign_discount_rate_source"});
            return;
        }
        return;
    }

    double ForwardCurveFX::getForward(Date toDate) const
    {
        return quotes.getForward(toDate);
    }

}
