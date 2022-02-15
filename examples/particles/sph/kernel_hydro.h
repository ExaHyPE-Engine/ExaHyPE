#ifndef PEANO_KERNEL_HYDRO_H
#define PEANO_KERNEL_HYDRO_H




#include "Constants.h"
#include "peano4/utils/Globals.h"

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
    w = (0.25 * (2.0 - q)*(2.0 - q)*(2.0 - q));
  } else {
    w = (1.0 - 1.5*q*q*(1.0 - q/2.0));
  }

  // Kernel derivative

  if (q >= 2) {
    dw_dq = 0.0;
  } else if (q >= 1) {
    dw_dq = -0.75 * ( (2 - q)*(2 - q) );
  } else {
    dw_dq = 0.75*q * (3*q - 4);
  }
  
  // Final expression (up to overall factors)
  // Done outside now.
//  dw_dq += Dimensions*w;

  // Return final expressions (up to overall factors)
  W     = w     * examples::particles::CUBIC_SPLINE_SIGMA; 
  dW_dq = dw_dq * examples::particles::CUBIC_SPLINE_SIGMA;



}

#endif // PEANO_KERNEL_HYDRO_H

