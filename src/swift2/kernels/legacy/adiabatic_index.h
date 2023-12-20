#pragma once

/**
 * @file adiabatic_index.h
 * @brief contains functions and definitions related to the adiabatic index
 * of the gas.
 * This is a legacy file - kept as closely to the SWIFT original files as possible.
 */

namespace swift2 {
  namespace kernels {
  namespace legacy {
    namespace adiabaticIndex {

/* Powers of the adiabatic index */
#if defined(HYDRO_GAMMA_5_3)

#define hydro_gamma 1.66666666666666667
#define hydro_gamma_minus_one 0.66666666666666667
#define hydro_gamma_plus_one 2.66666666666666667
#define hydro_one_over_gamma_minus_one 1.5
#define hydro_gamma_plus_one_over_two_gamma 0.8
#define hydro_gamma_minus_one_over_two_gamma 0.2
#define hydro_gamma_minus_one_over_gamma_plus_one 0.25
#define hydro_two_over_gamma_plus_one 0.75
#define hydro_two_over_gamma_minus_one 3.f
#define hydro_gamma_minus_one_over_two 0.33333333333333333
#define hydro_two_gamma_over_gamma_minus_one 5.
#define hydro_one_over_gamma 0.6

#elif defined(HYDRO_GAMMA_7_5)

#define hydro_gamma 1.4
#define hydro_gamma_minus_one 0.4
#define hydro_gamma_plus_one 2.4
#define hydro_one_over_gamma_minus_one 2.5
#define hydro_gamma_plus_one_over_two_gamma 0.857142857
#define hydro_gamma_minus_one_over_two_gamma 0.142857143
#define hydro_gamma_minus_one_over_gamma_plus_one 0.166666667
#define hydro_two_over_gamma_plus_one 0.83333333
#define hydro_two_over_gamma_minus_one 5.
#define hydro_gamma_minus_one_over_two 0.2
#define hydro_two_gamma_over_gamma_minus_one 7.
#define hydro_one_over_gamma 0.714285714

#elif defined(HYDRO_GAMMA_4_3)

#define hydro_gamma 1.33333333333333333
#define hydro_gamma_minus_one 0.33333333333333333
#define hydro_gamma_plus_one 2.33333333333333333
#define hydro_one_over_gamma_minus_one 3.
#define hydro_gamma_plus_one_over_two_gamma 0.875
#define hydro_gamma_minus_one_over_two_gamma 0.125
#define hydro_gamma_minus_one_over_gamma_plus_one 0.142857143
#define hydro_two_over_gamma_plus_one 0.857142857
#define hydro_two_over_gamma_minus_one 6.
#define hydro_gamma_minus_one_over_two 0.166666666666666666
#define hydro_two_gamma_over_gamma_minus_one 8.
#define hydro_one_over_gamma 0.75

#elif defined(HYDRO_GAMMA_2_1)

#define hydro_gamma 2.
#define hydro_gamma_minus_one 1.
#define hydro_gamma_plus_one 3.
#define hydro_one_over_gamma_minus_one 1.
#define hydro_gamma_plus_one_over_two_gamma 0.75
#define hydro_gamma_minus_one_over_two_gamma 0.25
#define hydro_gamma_minus_one_over_gamma_plus_one 0.33333333333333333
#define hydro_two_over_gamma_plus_one 0.66666666666666666
#define hydro_two_over_gamma_minus_one 2.
#define hydro_gamma_minus_one_over_two 0.5
#define hydro_two_gamma_over_gamma_minus_one 4.
#define hydro_one_over_gamma 0.5

#else

#error "An adiabatic index needs to be chosen!"

#endif

      /**
       * @brief Returns the argument to the power given by the adiabatic index
       *
       * Computes \f$x^\gamma\f$.
       */
      __attribute__((always_inline, const)) inline static double pow_gamma(double x) {

#if defined(HYDRO_GAMMA_5_3)

        const double cbrt = std::cbrt(x); /* x^(1/3) */
        return cbrt * cbrt * x;           /* x^(5/3) */

#elif defined(HYDRO_GAMMA_7_5)

        return std::pow(x, 1.4); /* x^(7/5) */

#elif defined(HYDRO_GAMMA_4_3)

        return std::cbrt(x) * x; /* x^(4/3) */

#elif defined(HYDRO_GAMMA_2_1)

        return x * x;

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Returns the argument to the power given by the adiabatic index minus
       * one
       *
       * Computes \f$x^{(\gamma-1)}\f$.
       */
      __attribute__((always_inline, const)) inline static double pow_gamma_minus_one(double x) {

#if defined(HYDRO_GAMMA_5_3)

        const double cbrt = std::cbrt(x); /* x^(1/3) */
        return cbrt * cbrt;               /* x^(2/3) */

#elif defined(HYDRO_GAMMA_7_5)

        return std::pow(x, 0.4); /* x^(2/5) */

#elif defined(HYDRO_GAMMA_4_3)

        return std::cbrt(x); /* x^(1/3) */

#elif defined(HYDRO_GAMMA_2_1)

        return x;

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Returns one over the argument to the power given by the adiabatic
       * index minus one
       *
       * Computes \f$x^{-(\gamma-1)}\f$.
       */
      __attribute__((always_inline, const)) inline static double pow_minus_gamma_minus_one(double x) {

#if defined(HYDRO_GAMMA_5_3)

        const double cbrt_inv = 1. / std::cbrt(x); /* x^(-1/3) */
        return cbrt_inv * cbrt_inv;                /* x^(-2/3) */

#elif defined(HYDRO_GAMMA_7_5)

        return std::pow(x, -0.4); /* x^(-2/5) */

#elif defined(HYDRO_GAMMA_4_3)

        return 1.f / std::cbrt(x); /* x^(-1/3) */

#elif defined(HYDRO_GAMMA_2_1)

        return 1.f / x;

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Returns one over the argument to the power given by the adiabatic
       * index
       *
       * Computes \f$x^{-\gamma}\f$.
       *
       * @param x Argument
       * @return One over the argument to the power given by the adiabatic index
       */
      __attribute__((always_inline, const)) inline static double pow_minus_gamma(double x) {

#if defined(HYDRO_GAMMA_5_3)

        const double cbrt_inv  = 1.f / std::cbrt(x);  /* x^(-1/3) */
        const double cbrt_inv2 = cbrt_inv * cbrt_inv; /* x^(-2/3) */
        return cbrt_inv * cbrt_inv2 * cbrt_inv2;      /* x^(-5/3) */

#elif defined(HYDRO_GAMMA_7_5)

        return std::pow(x, -1.4); /* x^(-7/5) */

#elif defined(HYDRO_GAMMA_4_3)

        const double cbrt_inv  = 1.f / std::cbrt(x);  /* x^(-1/3) */
        const double cbrt_inv2 = cbrt_inv * cbrt_inv; /* x^(-2/3) */
        return cbrt_inv2 * cbrt_inv2;                 /* x^(-4/3) */

#elif defined(HYDRO_GAMMA_2_1)

        const double inv = 1.f / x;
        return inv * inv;

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Return the argument to the power given by two divided by the adiabatic
       * index minus one
       *
       * Computes \f$x^{\frac{2}{\gamma - 1}}\f$.
       *
       * @param x Argument
       * @return Argument to the power two divided by the adiabatic index minus one
       */
      __attribute__((always_inline, const)) inline static double pow_two_over_gamma_minus_one(double x) {

#if defined(HYDRO_GAMMA_5_3)

        return x * x * x; /* x^3 */

#elif defined(HYDRO_GAMMA_7_5)

        const double x2 = x * x;
        const double x3 = x2 * x;
        return x2 * x3;

#elif defined(HYDRO_GAMMA_4_3)

        const double x3 = x * x * x; /* x^3 */
        return x3 * x3;              /* x^6 */

#elif defined(HYDRO_GAMMA_2_1)

        return x * x; /* x^2 */

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Return the argument to the power given by two times the adiabatic
       * index divided by the adiabatic index minus one
       *
       * Computes \f$x^{\frac{2\gamma}{\gamma - 1}}\f$.
       *
       * @param x Argument
       * @return Argument to the power two times the adiabatic index divided by the
       * adiabatic index minus one
       */
      __attribute__((always_inline, const)) inline static double pow_two_gamma_over_gamma_minus_one(double x) {

#if defined(HYDRO_GAMMA_5_3)

        const double x2 = x * x;
        const double x3 = x2 * x;
        return x2 * x3;

#elif defined(HYDRO_GAMMA_7_5)

        const double x2 = x * x;
        const double x4 = x2 * x2;
        return x4 * x2 * x;

#elif defined(HYDRO_GAMMA_4_3)

        const double x2 = x * x;
        const double x4 = x2 * x2;
        return x4 * x4; /* x^8 */

#elif defined(HYDRO_GAMMA_2_1)

        const double x2 = x * x;
        return x2 * x2; /* x^4 */

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Return the argument to the power given by the adiabatic index minus
       * one  divided by two times the adiabatic index
       *
       * Computes \f$x^{\frac{\gamma - 1}{2\gamma}}\f$.
       *
       * @param x Argument
       * @return Argument to the power the adiabatic index minus one divided by two
       * times the adiabatic index
       */
      __attribute__((always_inline, const)) inline static double pow_gamma_minus_one_over_two_gamma(double x) {

#if defined(HYDRO_GAMMA_5_3)

        return std::pow(x, 0.2); /* x^0.2 */

#elif defined(HYDRO_GAMMA_7_5)

        return std::pow(x, hydro_gamma_minus_one_over_two_gamma);

#elif defined(HYDRO_GAMMA_4_3)

        return std::pow(x, 0.125); /* x^0.125 */

#elif defined(HYDRO_GAMMA_2_1)

        return std::pow(x, 0.25); /* x^0.25 */

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Return the inverse argument to the power given by the adiabatic index
       * plus one divided by two times the adiabatic index
       *
       * Computes \f$x^{-\frac{\gamma + 1}{2\gamma}}\f$.
       *
       * @param x Argument
       * @return Inverse argument to the power the adiabatic index plus one divided by
       * two times the adiabatic index
       */
      __attribute__((always_inline, const)) inline static double pow_minus_gamma_plus_one_over_two_gamma(double x) {

#if defined(HYDRO_GAMMA_5_3)

        return std::pow(x, -0.8); /* x^-0.8 */

#elif defined(HYDRO_GAMMA_7_5)

        return std::pow(x, -hydro_gamma_plus_one_over_two_gamma);

#elif defined(HYDRO_GAMMA_4_3)

        return std::pow(x, -0.875); /* x^-0.875 */

#elif defined(HYDRO_GAMMA_2_1)

        return std::pow(x, -0.75); /* x^-0.75 */

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Return the argument to the power one over the adiabatic index
       *
       * Computes \f$x^{\frac{1}{\gamma}}\f$.
       *
       * @param x Argument
       * @return Argument to the power one over the adiabatic index
       */
      __attribute__((always_inline, const)) inline static double pow_one_over_gamma(double x) {

#if defined(HYDRO_GAMMA_5_3)

        return std::pow(x, hydro_one_over_gamma); /* x^(3/5) */

#elif defined(HYDRO_GAMMA_7_5)

        return std::pow(x, hydro_one_over_gamma);

#elif defined(HYDRO_GAMMA_4_3)

        return std::pow(x, hydro_one_over_gamma); /* x^(3/4) */

#elif defined(HYDRO_GAMMA_2_1)

        return std::sqrt(x); /* x^(1/2) */

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Return the argument to the power three adiabatic index minus two.
       *
       * Computes \f$x^{3\gamma - 2}\f$.
       *
       * @param x Argument
       */
      __attribute__((always_inline, const)) inline static double pow_three_gamma_minus_two(double x) {

#if defined(HYDRO_GAMMA_5_3)

        return x * x * x; /* x^(3) */

#elif defined(HYDRO_GAMMA_7_5)

        return std::pow(x, 2.2); /* x^(11/5) */

#elif defined(HYDRO_GAMMA_4_3)

        return x * x; /* x^(2) */

#elif defined(HYDRO_GAMMA_2_1)

        return x * x * x * x; /* x^(4) */

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Return the argument to the power three adiabatic index minus five over
       * two.
       *
       * Computes \f$x^{(3\gamma - 5)/2}\f$.
       *
       * @param x Argument
       */
      __attribute__((always_inline, const)) inline static double pow_three_gamma_minus_five_over_two(double x) {

#if defined(HYDRO_GAMMA_5_3)

        return 1.f; /* x^(0) */

#elif defined(HYDRO_GAMMA_7_5)

        return std::pow(x, -0.4); /* x^(-2/5) */

#elif defined(HYDRO_GAMMA_4_3)

        return 1.f / std::sqrt(x); /* x^(-1/2) */

#elif defined(HYDRO_GAMMA_2_1)

        return std::sqrt(x); /* x^(1/2) */

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

      /**
       * @brief Return the argument to the power three (adiabatic index - 1).
       *
       * Computes \f$x^{3(\gamma - 1)}\f$.
       *
       * @param x Argument
       */
      __attribute__((always_inline, const)) inline static double pow_three_gamma_minus_one(double x) {

#if defined(HYDRO_GAMMA_5_3)

        return x * x; /* x^(2) */

#elif defined(HYDRO_GAMMA_7_5)

        return std::pow(x, 1.2); /* x^(6/5) */

#elif defined(HYDRO_GAMMA_4_3)

        return x; /* x^(1) */

#elif defined(HYDRO_GAMMA_2_1)

        return x * x * x; /* x^(3) */

#else

        error("The adiabatic index is not defined !");
        return 0.f;

#endif
      }

    } // namespace adiabaticIndex
  }   // namespace legacy
  }
} // namespace swift2
