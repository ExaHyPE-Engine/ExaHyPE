// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_LA_SCALAR_H_
#define _TARCH_LA_SCALAR_H_


#include <limits>


namespace tarch {
  namespace la {
    constexpr double PI = 3.1415926535897932384626433832795028841972;
    constexpr double E = 2.7182818284590452353602874713526624977572;
    #ifdef MACHINE_PRECISION
    constexpr double NUMERICAL_ZERO_DIFFERENCE = MACHINE_PRECISION;
    #else
    constexpr double NUMERICAL_ZERO_DIFFERENCE = 1.0e-8;
    #endif

    /**
     * This routine takes NUMERICAL_ZERO_DIFFERENCE and scales it with value if
     * value is bigger than one. If it falls below one, then it returns the
     * original NUMERICAL_ZERO_DIFFERENCE.
     *
     * The version with two arguments, can be seen as
     * wrapper around a 'scalar' relativeEps() variant which forwards the
     * bigger value. This is very often used in combination with the greater
     * and smaller macros, where you wanna compare prescribing a comparison
     * tolerance.
     *
     * If you have only one argument, set valueB=valueA.
     */
    double relativeEps( double valueA, double valueB=std::numeric_limits<double>::min(), double eps = NUMERICAL_ZERO_DIFFERENCE );

    /**
     * I need the maximum of three values all the time, to I decided to write a
     * function for this.
     */
    double max( double a, double b, double c);

    /**
     * Wrapper around std::pow which is redirected to Intel's implementation on 
     * Intel machines.
     */
    double pow( double base, double exponent );

    /**
     * Convert an absolute value into a relative one. That is if the
     * referenceValue is smaller than one, then we do nothing. Otherwise,
     * we divide the absolute value by the reference value.
     */
    double convertAbsoluteIntoRelativeValue( double referenceValue, double value );
  }
}

#endif
