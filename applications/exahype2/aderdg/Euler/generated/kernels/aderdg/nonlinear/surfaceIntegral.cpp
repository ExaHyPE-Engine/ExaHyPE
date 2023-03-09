
#include "./Kernels.h"
#include "./DGMatrices.h"
#include "./Quadrature.h"


void generated::kernels::AderDG::nonlinear::surfaceIntegral( 
  double* __restrict__ lduh, 
  const double* __restrict__ const lFhbnd, 
  const double inverseDx //Assume dx[0] == dx[1] == dx[2]
) {

#ifdef __INTEL_COMPILER
  __assume_aligned(FRCoeff,  16);
  __assume_aligned(FLCoeff,  16);
  __assume_aligned(weights2, 16);
  __assume_aligned(lFhbnd,   16);
  __assume_aligned(lduh,     16);
#endif

  // x faces
  for (int yz = 0; yz < 4; yz++) {
    const double weight = weights2[yz] * inverseDx; //Assume dx[0] == dx[1] == dx[2]
    for (int x = 0; x < 4; x++) {
      #pragma omp simd aligned(lduh,lFhbnd:16)
      for (int n = 0; n < 4; n++) {
        lduh[n+4*(x+4*yz)] -= weight *
            (lFhbnd[n+4*yz+16] * FRCoeff[x] -              lFhbnd[n+4*yz+0] * FLCoeff[x]);
      }
    }
  }

  // y faces
  for (int xz = 0; xz < 4; xz++) {
    const double weight = weights2[xz] * inverseDx; //Assume dx[0] == dx[1] == dx[2]
    const int xzLuhIndex = xz*4;
    for (int y = 0; y < 4; y++) {
      #pragma omp simd aligned(lduh,lFhbnd:16)
      for (int n = 0; n < 4; n++) {
        lduh[n+xzLuhIndex+y*16] -= weight *
            (lFhbnd[n+4*xz+48] * FRCoeff[y] -
             lFhbnd[n+4*xz+32] * FLCoeff[y]);
        }
      }
  }
}
