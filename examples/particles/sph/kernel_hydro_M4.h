#ifndef PEANO_KERNEL_HYDRO_H
#define PEANO_KERNEL_HYDRO_H




#include "Constants.h"
#include "peano4/utils/Globals.h"
#include <math.h>

/*
 * This function takes q, and returns W(q) and W'(q)=dW/dq.
 * We do include the normalisation factor of the kernel,
 * but notice that h-factors have to be included outside.
 *
 *  The derivative W'(q) is frequently used to construct others.
 *  For example:
 *
 *  dW_dh = - ( d*W + q*dW_dq ) / h^d+1
 *  dW_dr =  dW_dq / h^d+1
 *
 * where d=Dimensions.
 *
 *
 */


__attribute__((always_inline)) inline static void kernel_deval(
  double q, double &W, double &dW_dq){

  double w, dw_dq;

  // Default SPH kernel (cubic spline)

  if (q >= 2.0) {
    w = 0.0;
  } else if (q >= 1.0) {
    w = 0.25 * (2.0 - q) * (2.0 - q) * (2.0 - q);
  } else {
    w = 1.0 - 1.5 * q * q * (1.0 - q / 2.0);
  }

  // Kernel derivative

  if (q >= 2.0) {
    dw_dq = 0.0;
  } else if (q >= 1.0) {
    dw_dq = - 0.75 * (2.0 - q) * (2.0 - q);
  } else {
    dw_dq = 0.75 * q * (3.0 * q - 4.0);
  }
  
  // Final expression (up to overall factors)
  // Done outside now.
//  dw_dq += Dimensions*w;

  // Return final expressions (up to overall factors)
  W     = w     * examples::particles::CUBIC_SPLINE_SIGMA; 
  dW_dq = dw_dq * examples::particles::CUBIC_SPLINE_SIGMA;

}

/*****************************************************************************/
/*****************************************************************************/
/* M4 KERNEL FROM SWIFT */

/* Coefficients for the kernel. */
#define kernel_degree 3 /*!< Degree of the polynomial */
#define kernel_ivals 2  /*!< Number of branches */

/* 1D */
#define kernel_gamma ((float)(1.732051))
#define kernel_constant ((float)(8. / 3.))
/* 2D */
//#define kernel_gamma ((float)(1.778002))
//#define kernel_constant ((float)(80. / M_PI / 7.))

/* First some powers of gamma = H/h */
#define kernel_gamma_inv ((float)(1. / kernel_gamma))
#define kernel_gamma2 ((float)(kernel_gamma * kernel_gamma))

/* define gamma^d, gamma^(d+1), 1/gamma^d and 1/gamma^(d+1) */

/* 1D */
#define kernel_gamma_dim ((float)(kernel_gamma))
#define kernel_gamma_dim_plus_one ((float)(kernel_gamma * kernel_gamma))
#define kernel_gamma_inv_dim ((float)(1. / (kernel_gamma)))
#define kernel_gamma_inv_dim_plus_one \
  ((float)(1. / (kernel_gamma * kernel_gamma)))

/* 2D */
//#define kernel_gamma_dim ((float)(kernel_gamma * kernel_gamma))
//#define kernel_gamma_dim_plus_one \
//  ((float)(kernel_gamma * kernel_gamma * kernel_gamma))
//#define kernel_gamma_inv_dim ((float)(1. / (kernel_gamma * kernel_gamma)))
//#define kernel_gamma_inv_dim_plus_one \
//  ((float)(1. / (kernel_gamma * kernel_gamma * kernel_gamma)))


/* The number of branches (floating point conversion) */
#define kernel_ivals_f ((float)(kernel_ivals))

static const double kernel_coeffs[(kernel_degree + 1) * (kernel_ivals + 1)]
    __attribute__((aligned(16))) = {3.f,  -3.f, 0.f,  0.5f, /* 0 < u < 0.5 */
                                    -1.f, 3.f,  -3.f, 1.f,  /* 0.5 < u < 1 */
                                    0.f,  0.f,  0.f,  0.f}; /* 1 < u */

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
__attribute__((always_inline)) inline static void kernel_eval(
    double u, double &W) {

  /* Go to the range [0,1[ from [0,H[ */
  const double x = u * kernel_gamma_inv;

  /* Pick the correct branch of the kernel */
  const int temp = (int)(x * kernel_ivals_f);
  const int ind = temp > kernel_ivals ? kernel_ivals : temp;
  const double *const coeffs = &kernel_coeffs[ind * (kernel_degree + 1)];

  /* First two terms of the polynomial ... */
  double w = coeffs[0] * x + coeffs[1];

  /* ... and the rest of them */
  for (int k = 2; k <= kernel_degree; k++) w = x * w + coeffs[k];

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
 * @param u The ratio of the distance to the smoothing length \f$u = x/h\f$.
 * @param W (return) The value of the kernel function \f$W(x,h)\f$.
 * @param dW_dx (return) The norm of the gradient of \f$|\nabla W(x,h)|\f$.
 */
__attribute__((always_inline)) inline static void kernel_deval_SWIFT(
    double u, double &W, double &dW_dx) {

  /* Go to the range [0,1[ from [0,H[ */
  const double x = u * kernel_gamma_inv;

  /* Pick the correct branch of the kernel */
  const int temp = (int)(x * kernel_ivals_f);
  const int ind = temp > kernel_ivals ? kernel_ivals : temp;
  const double *const coeffs = &kernel_coeffs[ind * (kernel_degree + 1)];

  /* First two terms of the polynomial ... */
  double w = coeffs[0] * x + coeffs[1];
  double dw_dx = coeffs[0];

  /* ... and the rest of them */
  for (int k = 2; k <= kernel_degree; k++) {
    dw_dx = dw_dx * x + w;
    w = x * w + coeffs[k];
  }

  w = std::max(w, 0.0);
  dw_dx = std::min(dw_dx, 0.0);

  /* Return everything */
  W = w * kernel_constant * kernel_gamma_inv_dim;
  dW_dx = dw_dx * kernel_constant * kernel_gamma_inv_dim_plus_one;
}






#endif // PEANO_KERNEL_HYDRO_H

