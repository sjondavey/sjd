#include <sjd/maths/maths.h>

#include <sjd/tools/Errors.h>

#include <limits>

//======================================================================================
// MathsFunctions
//======================================================================================
using namespace std;

namespace sjd
{

    unsigned long int factorial(unsigned int a)
    {
        if (a == 0)
            return 1;
        int fact = 1;
        for (int i = a; i > 0; --i)
        {
            fact *= i;
        }
        return fact;
    }

    unsigned long int combination(unsigned int a, unsigned int b)
    {
        int comb = 1;
        if (a == b)
        {
            return 1;
        }
        else if (b >= (a - b))
        {
            for (unsigned int i = a; i > b; --i)
            {
                comb *= i;
            }
            comb /= factorial(a - b);
            return comb;
        }
        else if (b < (a - b))
        {
            for (unsigned int i = a; i > (a - b); --i)
            {
                comb *= i;
            }
            comb /= factorial(b);
            return comb;
        }
        return 0;
    }

    //======================================================================================
    // TwoPointInterpolator
    //======================================================================================
    TwoPointInterpolator::TwoPointInterpolator(double x1, double y1, double x2, double y2)
        : x1(x1), x2(x2), y1(y1), y2(y2)
    {
    }

    bool TwoPointInterpolator::arePointsDistinct() const
    {
        if (x1 == x2)
        {
            return false;
        }
        return true;
    }

    bool TwoPointInterpolator::doesExtrapolate(double x) const
    {
        if (x1 <= x2)
        {
            if (!((x1 <= x) && (x <= x2)))
            {
                return true;
            }
        }
        else
        {
            if (!((x2 <= x) && (x <= x1)))
            {
                return true;
            }
        }
        return false;
    }

    //======================================================================================
    // LinearInterpolator
    //======================================================================================
    LinearInterpolator::LinearInterpolator(double x1, double y1, double x2, double y2)
        : TwoPointInterpolator(x1, y1, x2, y2)
    {
    }

    double LinearInterpolator::getRate(double x) const
    {
        double m = (y2 - y1) / (x2 - x1);
        double c = y1 - m * x1;
        return m * x + c;
    }

    //======================================================================================
    // ArrayInterpolator
    //======================================================================================
    ArrayInterpolator::ArrayInterpolator(vector<double> xVector,
                                         vector<double> yVector,
                                         bool allowExtrapolation)
        : xVector(xVector), yVector(yVector), allowExtrapolation(allowExtrapolation)
    {
        errorTracking = std::make_shared<IssueTracker>("ArrayInterpolator");
        validate();
    }

    void ArrayInterpolator::runChecks()
    {
        if (!isStrictlyIncreasing(xVector))
        {
            recordIssue(issues::INTERPOLATOR_X_NOT_INCREASING, "X vector input not strictly monotonic", {});
            return;
        }
        if (xVector.size() != yVector.size())
        {
            recordIssue(issues::INTERPOLATOR_DIMENSION_MISMATCH,
                        "X and Y vectors do not have the same dimension", {});
            return;
        }
        if (xVector.size() < 2)
        {
            recordIssue(issues::INTERPOLATOR_TOO_FEW_POINTS, "X vector must have at least 2 points", {});
        }
    }

    void ArrayInterpolator::setXVector(vector<double> xVectorInput)
    {
        xVector = xVectorInput;
        validate();
    }

    void ArrayInterpolator::setYVector(vector<double> yVectorInput)
    {
        yVector = yVectorInput;
        validate();
    }

    bool ArrayInterpolator::isInRange(double x) const
    {
        if (!isOK() || xVector.empty())
        {
            return false;
        }
        if (allowExtrapolation)
        {
            return true;
        }
        else if ((x >= xVector.front()) && (x <= xVector.back()))
        {
            return true;
        }
        return false;
    }

    bool ArrayInterpolator::isInRange(vector<double> xInput) const
    {
        if (!isOK())
        {
            return false;
        }
        if (allowExtrapolation)
        {
            return true;
        }
        bool allInRange = true;
        for (vector<double>::iterator it = xInput.begin(); it != xInput.end(); ++it)
        {
            double x = *it;
            if (!isInRange(x))
            {
                allInRange = false;
            }
        }
        return allInRange;
    }

    vector<double> ArrayInterpolator::getRate(vector<double> xInput) const
    {
        vector<double> yOutput;
        for (vector<double>::iterator it = xInput.begin(); it != xInput.end(); ++it)
        {
            double x = *it;
            yOutput.push_back(getRate(x));
        }
        return yOutput;
    }

    //======================================================================================
    // LinearArrayInterpolator
    //======================================================================================
    LinearArrayInterpolator::LinearArrayInterpolator(vector<double> xVector,
                                                     vector<double> yVector,
                                                     bool inputAllowExtrapolation)
        : ArrayInterpolator(xVector, yVector, inputAllowExtrapolation)
    {
    }

    double LinearArrayInterpolator::getRate(double x) const
    {
        SJD_ASSERT(isOK());
        SJD_ASSERT(allowExtrapolation || isInRange(x));
        // we are now either inside the range (if we do not allow extrapolation) or we allow extrapolation
        // using the "closest" points in the input arrays

        // We find the right place in the table by means of bisection. This will return the two "extreme"
        // points if we are extrapolating outside the curve
        int ilo = 0;
        int ihi = static_cast<int>(xVector.size()) - 1;
        int i;
        while (ihi - ilo > 1)
        {
            i = (ihi + ilo) >> 1;
            if (xVector[i] > x)
            {
                ihi = i;
            }
            else
            {
                ilo = i;
            }
        }
        double h = xVector[ihi] - xVector[ilo];
        SJD_ASSERT(h != 0);
        return LinearInterpolator(xVector[ilo], yVector[ilo], xVector[ihi], yVector[ihi]).getRate(x);
    }

    //======================================================================================
    // CubicSplineInterpolator
    //======================================================================================
    CubicSplineInterpolator::CubicSplineInterpolator(
        vector<double> xVector, vector<double> yVector, double yp1, double ypn, bool allowExtrapolation)
        : ArrayInterpolator(xVector, yVector, allowExtrapolation), _yp1(yp1), _ypn(ypn),
          spline(xVector.size())
    {
        if (isOK())
        {
            setSpline();
        }
    }

    CubicSplineInterpolator::CubicSplineInterpolator(std::vector<double> xVector,
                                                     std::vector<double> yVector,
                                                     bool allowExtrapolation)
        : ArrayInterpolator(xVector, yVector, allowExtrapolation), _yp1(0.0), _ypn(0.0),
          spline(xVector.size())
    {
        if (isOK())
        {
            setSpline();
        }
    }

    double CubicSplineInterpolator::getRate(double x) const
    {
        SJD_ASSERT(isOK());
        SJD_ASSERT(allowExtrapolation || isInRange(x));

        // We find the right place in the table by means of bisection. This will return the two "extreme"
        // points if we are extrapolating outside the curve
        int klo = 0;
        int khi = static_cast<int>(spline.size()) - 1;
        int k;
        while (khi - klo > 1)
        {
            k = (khi + klo) >> 1;
            if (xVector[k] > x)
            {
                khi = k;
            }
            else
            {
                klo = k;
            }
        }

        double h = xVector[khi] - xVector[klo];
        SJD_ASSERT(h != 0);
        double a = (xVector[khi] - x) / h;
        double b = (x - xVector[klo]) / h;
        double y = a * yVector[klo] + b * yVector[khi] +
                   ((a * a * a - a) * spline[klo] + (b * b * b - b) * spline[khi]) * (h * h) / 6.0;
        return y;
    }

    void CubicSplineInterpolator::setSpline()
    {
        int n = static_cast<int>(spline.size());
        std::vector<double> tempCalcuation(n);
        spline[0] = -.5;
        if (_yp1 == 0)
        {
            tempCalcuation[0] = 0.0;
        }
        else
        {
            tempCalcuation[0] = (3.0 / (xVector[1] - xVector[0])) *
                                ((yVector[1] - yVector[0]) / (xVector[1] - xVector[0]) - _yp1);
        }

        double sig, p;
        for (int i = 1; i < n - 1; ++i)
        {
            sig = (xVector[i] - xVector[i - 1]) / (xVector[i + 1] - xVector[i - 1]);
            p = sig * spline[i - 1] + 2.0;
            spline[i] = (sig - 1.0) / p;
            tempCalcuation[i] = (yVector[i + 1] - yVector[i]) / (xVector[i + 1] - xVector[i]) -
                                (yVector[i] - yVector[i - 1]) / (xVector[i] - xVector[i - 1]);
            tempCalcuation[i] =
                (6.0 * tempCalcuation[i] / (xVector[i + 1] - xVector[i - 1]) - sig * tempCalcuation[i - 1]) /
                p;
        }
        double qn, un;
        if (_ypn == 0)
        {
            qn = un = 0;
        }
        else
        {
            qn = 0.5;
            un = (3.0 / (xVector[n - 1] - xVector[n - 2])) *
                 (_ypn - (yVector[n - 1] - yVector[n - 2]) / (xVector[n - 1] - xVector[n - 2]));
        }
        spline[n - 1] = (un - qn * tempCalcuation[n - 2]) / (qn * spline[n - 2] + 1.0);
        for (int k = n - 2; k >= 0; k--)
        {
            spline[k] = spline[k] * spline[k + 1] + tempCalcuation[k];
        }
    }

}