#pragma once
#include "cmath"
#include "tarch/tarch.h"

/**
 * @file kernel_hydro.h
 * @brief Kernel functions for SPH.
 *
 * Constants and kernel coefficients are taken from table 1 of
 * Dehnen & Aly, MNRAS, 425, pp. 1062-1082 (2012).
 *
 * This is a legacy file - kept as closely to the SWIFT original files as possible.
 */

#include "peano4/utils/Globals.h"
#include "tarch/compiler/CompilerSpecificSettings.h"


namespace swift2 {
  namespace kernels {
  namespace legacy {
    namespace kernelHydro {

#if defined(QUARTIC_SPLINE_KERNEL)
      /* ------------------------------------------------------------------------- */

      /* Quartic spline M5 kernel -- SWIFT version */

#define kernel_name "Quartic spline (M5)"
/* Coefficients for the kernel. */
#define kernel_degree 4 /* Degree of the polynomial */
#define kernel_ivals 5  /* Number of branches */

#if HYDRO_DIMENSION == 1
#define kernel_gamma ((float)(1.936492))
#define kernel_constant ((float)(3125. / 768.))
#elif HYDRO_DIMENSION == 2
#define kernel_gamma ((float)(1.977173))
#define kernel_constant ((float)(46875. * M_1_PI / 2398.))
#elif HYDRO_DIMENSION == 3
#define kernel_gamma ((float)(2.018932))
#define kernel_constant ((float)(15625. * M_1_PI / 512.))
#else
#pragma error "Hydro dimension undefined"
#endif

      static const double kernel_coeffs[(kernel_degree + 1) * (kernel_ivals + 1)] __attribute__((aligned(AlignmentOnHeap
      )))
      = {
        6.0,  0.0,  -2.4, 0.0,  0.368, /* 0 < u < 0.2 */
        -4.0, 8.0,  -4.8, 0.32, 0.352, /* 0.2 < u < 0.4 */
        -4.0, 8.0,  -4.8, 0.32, 0.352, /* 0.4 < u < 0.6 */
        1.0,  -4.0, 6.0,  -4.0, 1.0,   /* 0.6 < u < 0.8 */
        1.0,  -4.0, 6.0,  -4.0, 1.0,   /* 0.8 < u < 1 */
        0.0,  0.0,  0.0,  0.0,  0.0};  /* 1 < u */

/* ------------------------------------------------------------------------- */

// @TODO implement additional kernels (M4)
#else

#error "An SPH projection kernel function must be chosen !"
/* ------------------------------------------------------------------------- */
#endif

/* ------------------------------------------------------------------------- */

/* Define generic values (i.e. common to all kernels) */

/* First some powers of gamma = H/h */
#define kernel_gamma_inv ((float)(1. / kernel_gamma))
#define kernel_gamma2 ((float)(kernel_gamma * kernel_gamma))

/* define gamma^d, gamma^(d+1), 1/gamma^d and 1/gamma^(d+1) */
#if HYDRO_DIMENSION == 1
#define kernel_gamma_dim ((float)(kernel_gamma))
#define kernel_gamma_dim_plus_one ((float)(kernel_gamma * kernel_gamma))
#define kernel_gamma_inv_dim ((float)(1. / (kernel_gamma)))
#define kernel_gamma_inv_dim_plus_one ((float)(1. / (kernel_gamma * kernel_gamma)))
#elif HYDRO_DIMENSION == 2
#define kernel_gamma_dim ((float)(kernel_gamma * kernel_gamma))
#define kernel_gamma_dim_plus_one ((float)(kernel_gamma * kernel_gamma * kernel_gamma))
#define kernel_gamma_inv_dim ((float)(1. / (kernel_gamma * kernel_gamma)))
#define kernel_gamma_inv_dim_plus_one ((float)(1. / (kernel_gamma * kernel_gamma * kernel_gamma)))
#elif HYDRO_DIMENSION == 3
#define kernel_gamma_dim ((float)(kernel_gamma * kernel_gamma * kernel_gamma))
#define kernel_gamma_dim_plus_one ((float)(kernel_gamma * kernel_gamma * kernel_gamma * kernel_gamma))
#define kernel_gamma_inv_dim ((float)(1. / (kernel_gamma * kernel_gamma * kernel_gamma)))
#define kernel_gamma_inv_dim_plus_one ((float)(1. / (kernel_gamma * kernel_gamma * kernel_gamma * kernel_gamma)))
#endif

/* The number of branches (floating point conversion) */
#define kernel_ivals_f ((float)(kernel_ivals))

/* Kernel self contribution (i.e. W(0,h)) */
#define kernel_root ((float)(kernel_coeffs[kernel_degree]) * kernel_constant * kernel_gamma_inv_dim)

      /* ------------------------------------------------------------------------- */

      /**
       * @brief Computes the kernel function.
       *
       * The kernel function needs to be mutliplied by \f$h^{-d}\f$,
       * where \f$d\f$ is the dimensionality of the problem.
       *
       * Returns 0 if \f$u > \gamma = H/h\f$
       *
       * @param u The ratio of the distance to the smoothing length \f$u = x/h\f$.
       * @param W (return) The value of the kernel function \f$W(x,h)\f$.
       */
      static void kernel_eval(double u, double& W) InlineMethod {

        /* Go to the range [0,1[ from [0,H[ */
        const double x = u * kernel_gamma_inv;

        /* Pick the correct branch of the kernel */
        const int           temp   = (int)(x * kernel_ivals_f);
        const int           ind    = temp > kernel_ivals ? kernel_ivals : temp;
        const double* const coeffs = &kernel_coeffs[ind * (kernel_degree + 1)];

        /* First two terms of the polynomial ... */
        double w = coeffs[0] * x + coeffs[1];

        /* ... and the rest of them */
        for (int k = 2; k <= kernel_degree; k++)
          w = x * w + coeffs[k];

        w = std::max(w, 0.0);

        /* Return everything */
        W = w * kernel_constant * kernel_gamma_inv_dim;
      }

      /**
       * @brief Computes the kernel function and its derivative.
       *
       * The kernel function needs to be mutliplied by \f$h^{-d}\f$ and the gradient
       * by \f$h^{-(d+1)}\f$, where \f$d\f$ is the dimensionality of the problem.
       *
       * Returns 0 if \f$u > \gamma = H/h\f$.
       *
       *
       *
       * ## Implementation of polynomial evaluation
       *
       * We had originally the following calculation to compute w and dw_dx:
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * double w     = coeffs[0] * x + coeffs[1];
       * double dw_dx = coeffs[0];
       *
       * for (int k = 2; k <= kernel_degree; k++) {
       *   dw_dx = dw_dx * x + w;
       *   w     = x * w + coeffs[k];
       * }
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * This variant does not vectorise for multiple reasons: We accumulate in a
       * complicated way in w, which at least confuses the Intel 2023.2 compiler,
       * and then we have dependencies between dw_dx and w, i.e. w feeds into
       * dw_dx. So the idea to fuse two evaluation of Horner's method and to use
       * the same idea of Horner (re-use of lower-order polynomials) between two
       * quantities is contraproductive here.
       *
       * We keep the plain Horner scheme for the polynomial w:
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *  double w     = coeffs[0];
       *  for (int k = 1 ; k <= kernel_degree; k++) {
       *    w = w * x + coeffs[k];
       *  }
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * This part vectorises just fine and evaluates
       *
       * @f$ w = c_{p} + x \left( c_{p-1} + x \left( \left( c_{p-2} + x \left( ... \right) \right) \right) \right) @f$
       *
       * For the derivative, we can start from Horner's scheme, which naturally leads
       * to the vanilla implementation above. However, it is better to start from the
       * plain polynomial write-up
       *
       * @f{eqnarray*}{
       *   w & = & c_{0}x^p + c_{1}x^{p-1} + c_{2}x^{p-2} + ... + c_{p} \\
       *   \frac{\partial }{ \partial x } w & = & pc_{0}x^{p-1} + (p-1)c_{1}x^{p-2} + (p-2)c_{2}x^{p-3} + ... + c_{p-1}
       * \\ & = & (((((( p \cdot c_{0} ) x + (p-1)c_{1} ) x + (p-2)c_{2} ) x ... ) x + (p-(p-1)) c_{p-1}
       * @f}
       *
       * which we then have rewritten into Horner's scheme again.
       *
       *
       *
       * @param u The ratio of the distance to the smoothing length \f$u = x/h\f$.
       * @param W (return) The value of the kernel function \f$W(x,h)\f$.
       * @param dW_dx (return) The norm of the gradient of \f$|\nabla W(x,h)|\f$.
       */
      static void kernel_deval(double u, double& W, double& dW_dx) InlineMethod {
        /* Go to the range [0,1[ from [0,H[ */
        const double x = u * kernel_gamma_inv;

        /* Pick the correct branch of the kernel */
        const int           temp   = (int)(x * kernel_ivals_f);
        const int           ind    = temp > kernel_ivals ? kernel_ivals : temp;
        const double* const coeffs = &kernel_coeffs[ind * (kernel_degree + 1)];

        /* w term in Horner's scheme */
        double w = coeffs[0];
        for (int k = 1; k <= kernel_degree; k++) {
          w = w * x + coeffs[k];
        }

        /* dw_dx term in Horner's scheme */

        double dw_dx = coeffs[0] * kernel_degree;

        /* ... and the rest of them */
        for (int k = 1; k <= kernel_degree - 1; k++) {
          dw_dx = dw_dx * x + coeffs[k] * (kernel_degree - k);
        }

        w     = std::max(w, 0.0);
        dw_dx = std::min(dw_dx, 0.0);

        /* Return everything */
        W     = w * kernel_constant * kernel_gamma_inv_dim;
        dW_dx = dw_dx * kernel_constant * kernel_gamma_inv_dim_plus_one;
      }


    } // namespace kernelHydro
  }   // namespace legacy
  }
} // namespace swift2
