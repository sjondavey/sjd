#ifndef SJD_TWODIMINTERP_INCLUDED
#define SJD_TWODIMINTERP_INCLUDED
#pragma once

#include "maths.h"
#include <ql/math/interpolations/all.hpp>

namespace sjd
{

    /*======================================================================================
    TwoDimensionalInterpolation

     Abstract Base Class for 2 dimensional interpolation
    ======================================================================================*/
    class TwoDimensionalInterpolator
    {
    public:
        TwoDimensionalInterpolator(std::vector<double> xVector,
                                   std::vector<double> yVector,
                                   QuantLib::Matrix zMatrix,
                                   bool extrapolate = false)
            : x(xVector), y(yVector), z(zMatrix), allowExtrapolation(extrapolate)
        {
        }

        virtual ~TwoDimensionalInterpolator() {};

        virtual void setX(std::vector<double> xVector) { x = xVector; };
        virtual void setY(std::vector<double> yVector) { y = yVector; };
        virtual void setZ(QuantLib::Matrix zMatrix) { z = zMatrix; };

        virtual bool isOk();
        std::string getErrorMessage() const { return errorMessage; };

        // Returns true if the input (x, y) point is inside the interpolation range
        bool isInRange(double x, double y) const;
        // I assume yu have called isInRange(x, y) by this stage if you need to
        virtual double getRate(double x, double y) const = 0;

        // given a point (xInput, yInput) we use the following methods to find the "boundary"
        // (locateX(xInput), locateY(yInput)+1)       (locateX(xInput)+1, locateY(yInput)+1)
        // (locateX(xInput), locateY(yInput))         (locateX(xInput)+1, locateY(yInput))
        size_t locateX(double xInput) const;
        size_t locateY(double yInput) const;

        double getXStart() const { return x.front(); };
        double getXEnd() const { return x.back(); };
        double getYStart() const { return y.front(); };
        double getYEnd() const { return y.back(); };

    protected:
        void setOnError(std::string errorMessage);

        std::vector<double> x, y;
        QuantLib::Matrix z;

        bool allowExtrapolation;
        bool hasError; // used if any of the inputs are not of the assumed type
        std::string errorMessage;
    };

    /*======================================================================================
    BilinearInterpolator

    ======================================================================================*/
    class BilinearInterpolator : public TwoDimensionalInterpolator
    {
    public:
        BilinearInterpolator(std::vector<double> xVector,
                             std::vector<double> yVector,
                             QuantLib::Matrix zMatrix,
                             bool extrapolate)
            : TwoDimensionalInterpolator(xVector, yVector, zMatrix, extrapolate) {};

        ~BilinearInterpolator() {};

        virtual double getRate(double x, double y) const;
    };

    /*======================================================================================
    BicubicInterpolator

    ======================================================================================*/
    class BicubicInterpolator : public TwoDimensionalInterpolator
    {
    public:
        BicubicInterpolator(std::vector<double> xVector,
                            std::vector<double> yVector,
                            QuantLib::Matrix zMatrix,
                            bool extrapolate);

        ~BicubicInterpolator() {};

        virtual double getRate(double x, double y) const;

    protected:
        // std::vector<CubicSplineInterpolator> splines;
        std::vector<QuantLib::Interpolation> splines;
    };
}

#endif
