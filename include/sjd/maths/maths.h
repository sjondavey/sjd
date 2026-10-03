#ifndef SJD_MATHS_INCLUDED
#define SJD_MATHS_INCLUDED
#pragma once

/**
* The maths.h file adds functionality to the standard math.h file. It
* also uses the standard namespace so that NaN's can be returned by
* some of the functions (this is necessary when the files are used in 
* excel). This file also includes interpolation functions.
*/

//#include <math.h>
//#include <limits>
//#include <algorithm> // max / min functions
#include <string>
#include <vector>

namespace sjd {

   
   //sign(...): Use the function in "boost/math/special_functions"


   //Returns a! = (a)x(a-1)x(a-2)...(3)x(2)x(1)
   unsigned long int factorial(unsigned int a);
   
   //Returns a! / (b! * (a-b)!)   
   // if b > a this method returns 0
   unsigned long int combination(unsigned int a, unsigned int b);

   template <typename T>
   bool isStrictlyIncreasing(std::vector<T> data) 
    {
      bool strictlyIncreasing = true;
      size_t arrayLength = data.size();
      if (arrayLength < 1)
        {
         return false;
        }
        else if (arrayLength == 1)
        {
            return true;
        }
      for (size_t i = 0; (i < arrayLength - 1) && strictlyIncreasing; ++i) 
        {
         if (data[i] >= data[i+1])
            {
            strictlyIncreasing = false;
            }
      }
      return strictlyIncreasing;
   }

   template <typename T>
   bool isStrictlyDecreasing(std::vector<T> data)
   {
      bool strictlyDecreasing = true;
      size_t arrayLength = data.size();
      if (arrayLength < 1)
        {
         return false;
        }
        else if (arrayLength == 1)
        {
            return true;
        }
      for (size_t i = 0; (i < arrayLength - 1) && strictlyDecreasing; ++i) 
        {
         if (data[i] <= data[i+1])
            {
            strictlyDecreasing = false;
            }
      }
      return strictlyDecreasing;
   }

   template <typename T>
   bool isStrictlyMonotonic(std::vector<T> data) 
    {
      if (data.size() <= 1)
        {
         return false;
        }
      if (data[0] < data[1])      // increasing
        {
         return isStrictlyIncreasing(data);
        }
      else if (data[0] > data[1]) // decreasing
        {
         return isStrictlyDecreasing(data);
        }
      else                  // array[0] = array[1] 
        {
         return false;
        }
   }


   /*======================================================================================
   Interpolator 
   Abstract base class implemented by all Interpolators
   =======================================================================================*/
   class Interpolator
   {
   public:
      virtual double getRate(double x) const = 0;
   };

   /*======================================================================================
   TwoPointInterpolator 
   Abstract base class for all interpolators which use only two points to interpolate
   =======================================================================================*/
   class TwoPointInterpolator : public Interpolator
   {
   public:
      TwoPointInterpolator(double x1, double y1, double x2, double y2);
     
        // By default two point interpolators assume that the inputs x1 and x2 are 
        // different. If this is not the case there is a chance that the 
        // getRate(x) method could fail, badly. If you have any doubts about the
        // uniqueness of the inputs, use this method before calling getRate(x)
        bool arePointsDistinct() const;
      // Returns true if the input x is outside the range [x1, x2].
      bool doesExtrapolate(double x) const;

      virtual double getRate(double x) const = 0;

   protected:
      double x1, x2, y1, y2;
   };

   /*======================================================================================
   TwoPointInterpolator 
   A two point interpolator where rates are interpolated (or extrapolated) linearly
   =======================================================================================*/
   class LinearInterpolator : public TwoPointInterpolator
   {
   public:
      LinearInterpolator(double x1, double y1, double x2, double y2);
      virtual double getRate(double x) const;
   };


   /*======================================================================================
   As array interpolators are defined, add them here
   =======================================================================================*/
    enum ArrayInterpolatorType 
    {
        LINEAR, 
        CUBIC
    };

   /*======================================================================================
   ArrayInterpolator 
   This is built to accept x inputs that are strictly increasing 
   =======================================================================================*/
   class ArrayInterpolator : public Interpolator
   {
   public:
      ArrayInterpolator(std::vector<double> xVector, std::vector<double> yVector, bool allowExtrapolation = false);
      virtual ~ArrayInterpolator()   {};

   
      void setXVector(std::vector<double> xVector);
      void setYVector(std::vector<double> yVector);

      bool isOk();
      std::string getErrorMessage() const  {return errorMessage;};

      // Returns true if the input x is outside the range [x_min, x_max].
      bool isInRange(double x) const;
        // Returns true if ALL elements of the std::vector are in the range [x_min, x_max].
      bool isInRange(std::vector<double> x) const;

      virtual double getRate(double x) const = 0;
        // The default behaviour here iterates over each element in the std::vector
        // and then calls the getRate(x) method. This DOES NOT make use of the 
        // isInRange(...) function so please call this first if in doubt
        virtual std::vector<double> getRate(std::vector<double> x) const;

      double getRangeStart()   const {return xVector.front();};
      double getRangeEnd()   const {return xVector.back();};


   protected:
      void setOnError(std::string errorMessage);

      std::vector<double> xVector, yVector;
      bool allowExtrapolation;
      bool hasError; // used if any of the inputs are not of the assumed type
      std::string errorMessage;      
   };

   /*======================================================================================
   LinearArrayInterpolator 
   An array interpolator where rates are interpolated linearly. If extrapolation is allowed
   then the two points closest to the extrapolated value are used
   =======================================================================================*/
   class LinearArrayInterpolator : public ArrayInterpolator
   {
   public:
      LinearArrayInterpolator(std::vector<double> xVector, 
                                std::vector<double> yVector, 
                                bool inputAllowExtrapolation = false);    
      double getRate(double x) const;
        std::vector<double> getRate(std::vector<double> x) const {return ArrayInterpolator::getRate(x);};
   };

   /*======================================================================================
   CubicSplineInterpolator
   An array interpolator where rates are using the cubic spline methodology described in
   "Numerical Recipes in C". The goal of cubic spline interpolation is to get a interpolation
   formula that is smooth in the first derivative and continuous in the second derivative, 
   both within an interval and at its boundaries
   =======================================================================================*/
   class CubicSplineInterpolator : public ArrayInterpolator
   {
   public:
      // The cubic spline interpolator must be given first derivative conditions at its boundaries
      // y[0] and y[n-1]
      CubicSplineInterpolator(std::vector<double> xVector, 
                                std::vector<double> yVector, 
                                double yp1, 
                                double ypn, 
                                bool allowExtrapolation = false);
      // Use these constructors only if the first derivative at the boundaries are "natural", i.e. zero
      CubicSplineInterpolator(std::vector<double> xVector, 
                                std::vector<double> yVector, 
                                bool allowExtrapolation = false);

      double getRate(double x) const;
        std::vector<double> getRate(std::vector<double> x) const {return ArrayInterpolator::getRate(x);};

   private:
      /**
      * This function is only called once for the entire tabulated function
      *
      * Given arrays x and y containing the tabulated function y_i = f(x_i), i = 0,...,n and 
      * given values _yp1 and _ypn for the first derivative of the interpolating function at 
      * points 0 and n-1, this function sets the values for the std::vector spline that contains
      * the second derivative of the interpolating function at the tabulated points x_i. Setting
      * yp1 or ypn equal to 0 sets the boundary condition for a natural spline, with 0 second
      * derivative at that boundary
      */
      void setSpline();

      std::vector<double> spline;
      double _yp1; // the lower boundary condition which is std::set to be either "natural" or else to have a specified first derivative
      double _ypn;  // the upper boundary condition which is std::set to be either "natural" or else to have a specified first derivative
   };

}

#endif

