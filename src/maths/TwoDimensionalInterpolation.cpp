#include <sjd/maths/TwoDimensionalInterpolation.h>

#include <sjd/tools/Errors.h>

using namespace std;
using namespace QuantLib;

namespace sjd
{

    /*======================================================================================
    TwoDimensionalInterpolator

    ======================================================================================*/
    void TwoDimensionalInterpolator::runChecks()
    {
        if ((x.size() < 2) || (y.size() < 2) || (z.rows() < 2) || (z.columns() < 2))
        {
            recordIssue(issues::SURFACE_INPUTS_INSUFFICIENT,
                        "The input x, y or z data does not have sufficient data", {});
            return;
        }
        if (!isStrictlyIncreasing(x))
        {
            recordIssue(issues::SURFACE_X_NOT_INCREASING, "X inputs not strictly increasing", {});
            return;
        }
        if (!isStrictlyIncreasing(y))
        {
            recordIssue(issues::SURFACE_Y_NOT_INCREASING, "Y inputs not strictly increasing", {});
            return;
        }
        if (y.size() != z.columns())
        {
            recordIssue(issues::SURFACE_Y_DIMENSION, "Y vector has inconsistent dimension with z data", {});
            return;
        }
        if (x.size() != z.rows())
        {
            recordIssue(issues::SURFACE_X_DIMENSION, "X vector has inconsistent dimension with z data", {});
        }
    }

    bool TwoDimensionalInterpolator::isInRange(double xValue, double yValue) const
    {
        if (allowExtrapolation)
            return true;
        else if ((xValue >= getXStart()) && (xValue <= getXEnd()) && (yValue >= getYStart()) &&
                 (yValue <= getYEnd()))
            return true;
        return false;
    }

    size_t TwoDimensionalInterpolator::locateX(double xInput) const
    {
        if (xInput < getXStart())
            return 0;
        else if (xInput > getXEnd())
            return x.size() - 2;
        size_t index = 0;
        while ((index < x.size()) && (x[index] < xInput))
            ++index;
        if (index > 0)
            --index;
        return index;
    }

    size_t TwoDimensionalInterpolator::locateY(double yInput) const
    {
        if (yInput < getYStart())
            return 0;
        else if (yInput > getYEnd())
            return y.size() - 2;
        size_t index = 0;
        while ((index < y.size()) && (y[index] < yInput))
            ++index;
        if (index > 0)
            --index;
        return index;
    }

    /*======================================================================================
    BilinearInterpolator

    ======================================================================================*/
    double BilinearInterpolator::getRate(double xInput, double yInput) const
    {
        SJD_ASSERT(isInRange(xInput, yInput));

        size_t i = locateX(xInput);
        double x1 = x[i], x2 = x[i + 1];

        size_t j = locateY(yInput);
        double y1 = y[j], y2 = y[j + 1];

        double z1 = z[j][i], z2 = z[j + 1][i], z3 = z[j][i + 1], z4 = z[j + 1][i + 1];

        double t = (xInput - x1) / (x2 - x1);
        double u = (yInput - y1) / (y2 - y1);
        return (1 - t) * (1 - u) * z1 + t * (1 - u) * z3 + t * u * z4 + (1 - t) * u * z2;
    }

    /*======================================================================================
    BicubicInterpolator

    ======================================================================================*/
    BicubicInterpolator::BicubicInterpolator(vector<double> xVector,
                                             vector<double> yVector,
                                             Matrix zMatrix,
                                             bool extrapolate)
        : TwoDimensionalInterpolator(xVector, yVector, zMatrix, extrapolate)
    {
        if (!isOK())
            return;
        for (size_t i = 0; i < yVector.size(); ++i)
        {
            splines.push_back(CubicInterpolation(
                x.begin(), x.end(), this->z.row_begin(i), CubicInterpolation::Spline, false,
                CubicInterpolation::SecondDerivative, 0.0, CubicInterpolation::SecondDerivative, 0.0));
        }
    };

    double BicubicInterpolator::getRate(double xInput, double yInput) const
    {
        SJD_ASSERT(isInRange(xInput, yInput));

        std::vector<double> section(splines.size());
        for (Size i = 0; i < splines.size(); i++)
            section[i] = splines[i](xInput, allowExtrapolation);

        CubicInterpolation spline(y.begin(), y.end(), section.begin(), CubicInterpolation::Spline, false,
                                  CubicInterpolation::SecondDerivative, 0.0,
                                  CubicInterpolation::SecondDerivative, 0.0);
        return spline(yInput, true);
    }
}