#pragma once

#include "tarch/tarch.h" // for InlineMethod

/**
 * @file hydro_dimensions.h
 * @brief File containing macro-riddled functions related to dimensions.
 * This is a legacy file - kept as closely to the SWIFT original files as
 * possible.
 */

namespace swift2 {
  namespace kernels {
    namespace legacy {
      namespace hydroDimensions {

        /**
         * returns x^ndim
         */
        template <typename T> InlineMethod T pow_dimension(T x) {
#if (HYDRO_DIMENSION == 3)
          return x * x * x;
#elif (HYDRO_DIMENSION == 2)
          return x * x;
#elif (HYDRO_DIMENSION == 1)
          return x;
#else
#pragma error "HYDRO_DIMENSION undefined?"
#endif
        }

        /**
         * Returns \f$x^{1/d}\f$.
         */
        template <typename T> InlineMethod T pow_inv_dimension(T x) {
#if (HYDRO_DIMENSION == 3)
          return std::cbrt(x);
#elif (HYDRO_DIMENSION == 2)
          return std::sqrt(x);
#elif (HYDRO_DIMENSION == 1)
          return x;
#else
#pragma error "HYDRO_DIMENSION undefined?"
#endif
        }


        /**
         * returns x^{ndim+1}
         */
        template <typename T> InlineMethod T pow_dimension_plus_one(T x) {
#if (HYDRO_DIMENSION == 3)
          T temp = x * x;
          return temp * temp;
#elif (HYDRO_DIMENSION == 2)
          return x * x * x;
#elif (HYDRO_DIMENSION == 1)
          return x * x;
#else
#pragma error "HYDRO_DIMENSION undefined?"
#endif
        }


        /**
         * returns x^{ndim-1}
         */
        template <typename T> InlineMethod T pow_dimension_minus_one(T x) {
#if (HYDRO_DIMENSION == 3)
          return x * x;
#elif (HYDRO_DIMENSION == 2)
          return x;
#elif (HYDRO_DIMENSION == 1)
          return 1.;
#else
#pragma error "HYDRO_DIMENSION undefined?"
#endif
        }


      } // namespace hydroDimensions
    }   // namespace legacy
  }     // namespace kernels
} // namespace swift2
