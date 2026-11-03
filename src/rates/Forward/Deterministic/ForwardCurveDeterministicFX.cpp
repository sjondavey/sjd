#include <sjd/rates/Forward/Deterministic/ForwardCurveDeterministicFX.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    ForwardCurveDeterministicFX::ForwardCurveDeterministicFX(Date anchorDate,
                                                             double spot,
                                                             Date spotDate,
                                                             std::shared_ptr<DiscountRateSource> domesticDRS,
                                                             std::shared_ptr<DiscountRateSource> foreignDRS)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("ForwardCurveDeterministicFX");
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

    ForwardCurveDeterministicFX::ForwardCurveDeterministicFX(Date anchorDate,
                                                             double spotAtT0,
                                                             std::shared_ptr<DiscountRateSource> domesticDRS,
                                                             std::shared_ptr<DiscountRateSource> foreignDRS)
    {
        errorTracking = std::make_shared<sjd::IssueTracker>("ForwardCurveDeterministicFX");
        setParameters(anchorDate, spotAtT0, domesticDRS, foreignDRS);
    }

    void ForwardCurveDeterministicFX::setParameters(Date anchorDateInput,
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

    void ForwardCurveDeterministicFX::setSpotAtT0(double spotAtT0Input)
    {
        quotes.setSpotAtT0(spotAtT0Input);
    }

    double ForwardCurveDeterministicFX::getSpotAtT0()
    {
        return quotes.spotAtT0();
    }

    void ForwardCurveDeterministicFX::setRateSources(std::shared_ptr<DiscountRateSource> domesticDRSInput,
                                                     std::shared_ptr<DiscountRateSource> foreignDRSInput)
    {
        quotes.setRateSources(domesticDRSInput, foreignDRSInput);
        allowExtrapolation = quotes.allowsExtrapolation();
        flatBackFilled = quotes.anchorWasFlatFilled();
        finalDate = quotes.finalDate();
    }

    ForwardCurveDeterministicFX::~ForwardCurveDeterministicFX() {}

    void ForwardCurveDeterministicFX::runChecks()
    {
        ForwardRateSourceDeterministic::runChecks();
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

    double ForwardCurveDeterministicFX::getForward(Date toDate) const
    {
        return quotes.getForward(toDate);
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveDeterministicFX::parallelBump(double spread)
    {
        return quotes.parallelBump(spread);
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveDeterministicFX::rollForward(Date toDate)
    {
        return quotes.rollForward(toDate, isInRange(toDate));
    }

}
