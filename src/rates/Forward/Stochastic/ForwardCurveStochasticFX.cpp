#include <sjd/rates/Forward/Stochastic/ForwardCurveStochasticFX.h>

using namespace std;
using namespace QuantLib;
using namespace boost;

namespace sjd
{

    namespace
    {
        double spotOnAnchor(double spot,
                            Date spotDate,
                            const std::shared_ptr<DiscountRateSource>& domesticDRS,
                            const std::shared_ptr<DiscountRateSource>& foreignDRS)
        {
            if (!domesticDRS->extendsTo(spotDate) || !foreignDRS->extendsTo(spotDate))
            {
                return spot;
            }
            return spot * domesticDRS->getDiscountFactor(spotDate) / foreignDRS->getDiscountFactor(spotDate);
        }
    }

    ForwardCurveStochasticFX::ForwardCurveStochasticFX(Date anchorDate,
                                                       double spot,
                                                       Date spotDate,
                                                       std::shared_ptr<DiscountRateSource> domesticDRS,
                                                       std::shared_ptr<DiscountRateSource> foreignDRS,
                                                       std::shared_ptr<VolatilityRateSource> vrs)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("ForwardCurveStochasticFX"));
        setParameters(anchorDate, spotOnAnchor(spot, spotDate, domesticDRS, foreignDRS), domesticDRS,
                      foreignDRS, vrs);
    }

    ForwardCurveStochasticFX::ForwardCurveStochasticFX(Date anchorDate,
                                                       double spotAtT0,
                                                       std::shared_ptr<DiscountRateSource> domesticDRS,
                                                       std::shared_ptr<DiscountRateSource> foreignDRS,
                                                       std::shared_ptr<VolatilityRateSource> vrs)
    {
        errorTracking = std::shared_ptr<sjd::IssueTracker>(new sjd::IssueTracker("ForwardCurveStochasticFX"));
        setParameters(anchorDate, spotAtT0, domesticDRS, foreignDRS, vrs);
    }

    void ForwardCurveStochasticFX::setParameters(Date anchorDateInput,
                                                 double spotInput,
                                                 std::shared_ptr<DiscountRateSource> domesticDRSInput,
                                                 std::shared_ptr<DiscountRateSource> foreignDRSInput,
                                                 std::shared_ptr<VolatilityRateSource> vrs)
    {
        quotes.setInterestDifferential(anchorDateInput, spotInput, domesticDRSInput, foreignDRSInput,
                                       ForwardRoll::SpotVolatility, vrs);
        anchorDate = quotes.anchorDate();
        allowExtrapolation = quotes.allowsExtrapolation();
        finalDate = quotes.finalDate();
        setVolitalityRateSource(vrs);
    }

    void ForwardCurveStochasticFX::setSpotAtT0(double spotAtT0Input)
    {
        quotes.setSpotAtT0(spotAtT0Input);
    }

    double ForwardCurveStochasticFX::getSpotAtT0()
    {
        return quotes.spotAtT0();
    }

    void ForwardCurveStochasticFX::setRateSources(std::shared_ptr<DiscountRateSource> domesticDRSInput,
                                                  std::shared_ptr<DiscountRateSource> foreignDRSInput)
    {
        quotes.setRateSources(domesticDRSInput, foreignDRSInput);
        allowExtrapolation = quotes.allowsExtrapolation();
        finalDate = quotes.finalDate();
    }

    ForwardCurveStochasticFX::~ForwardCurveStochasticFX() {}

    bool ForwardCurveStochasticFX::isOK()
    {
        if (!ForwardRateSourceStochastic::isOK())
        {
            return false;
        }
        if (!quotes.domesticRateSource()->isOK())
        {
            absorbChildReport(quotes.domesticRateSource()->validationReport(),
                              "domestic_discount_rate_source");
            return false;
        }
        if (quotes.domesticRateSource()->getAnchorDate() != anchorDate)
        {
            recordIssue("FX_DOMESTIC_ANCHOR_MISMATCH", "Domestic DRS does not have the correct anchor date",
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
            recordIssue("FX_FOREIGN_ANCHOR_MISMATCH", "Foreign DRS does not have the correct anchor date",
                        {"foreign_discount_rate_source"});
            return false;
        }
        if (vrs->getAnchorDate() != anchorDate)
        {
            recordIssue("FX_VOLATILITY_ANCHOR_MISMATCH",
                        "Volatility Rate Source does not have the correct anchor date",
                        {"volatility_rate_source"});
            return false;
        }
        return !errorTracking->getHasErrors();
    }

    double ForwardCurveStochasticFX::getForward(Date toDate) const
    {
        return quotes.getForward(toDate);
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveStochasticFX::rollForward(Date toDate, double normalRV)
    {
        return quotes.rollForward(toDate, normalRV, isInRange(toDate));
    }

    std::shared_ptr<ForwardRateSource> ForwardCurveStochasticFX::rollForward(
        Date toDate, double normalRV, vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToDate)
    {
        return ForwardRateSourceStochastic::rollForward(toDate, normalRV, rateSetDates,
                                                        rateSetsFromAnchorToDate);
    }

    void ForwardCurveStochasticFX::setForwardValuationDates(set<Date> datesInput)
    {
        ForwardRateSourceStochastic::setForwardValuationDates(datesInput);
        if (forwardSimulationDates[0] < anchorDate || forwardSimulationDates.back() > finalDate)
        {
            forwardSimulationDates = vector<Date>();
            return;
        }
        quotes.preparePath(forwardSimulationDates, vector<Date>());
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurveStochasticFX::rollForward()
    {
        return quotes.rollPath(normalRVs, forwardSimulationDates);
    }

    vector<std::shared_ptr<ForwardRateSource>> ForwardCurveStochasticFX::rollForward(
        vector<Date> rateSetDates, vector<double>& rateSetsFromAnchorToFinalRollDate)
    {
        return ForwardRateSourceStochastic::rollForward(rateSetDates, rateSetsFromAnchorToFinalRollDate);
    }

}
