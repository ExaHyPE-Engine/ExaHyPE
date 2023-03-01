
#include "./Kernels.h"
#include "./DGMatrices.h"
#include "./Quadrature.h"


template<typename cCompType>
void generated::kernels::AderDG::nonlinear::faceIntegral(
                              cCompType *lduh, 
                              const cCompType *const lFhbnd,
                              const int direction, 
                              const int orientation,
                              const double inverseDxDirection
) {

  const double* const FCoeff = (orientation == 0 ? FLCoeff : FRCoeff);
  
#ifdef __INTEL_COMPILER
  __assume_aligned(FRCoeff,  16);
  __assume_aligned(FLCoeff,  16);
  __assume_aligned(FCoeff,   16);
  __assume_aligned(weights2, 16);
  __assume_aligned(lFhbnd,   16);
  __assume_aligned(lduh,     16);
#endif
  
  const cCompType scaling = (2.0 * orientation - 1.0) * inverseDxDirection;
  
  switch (direction) {
    case 0:   // x faces, left-right flux
      for (int zy = 0; zy < 4; zy++) { // zy
        const cCompType scaledWeight = scaling * weights2[zy];
        for (int x = 0; x < 4; x++) { // x
          #pragma omp simd aligned(lduh,FCoeff,lFhbnd:16)
          for (int n = 0; n < 4; n++) {
            lduh[(zy*4+x)*4+n] -= scaledWeight * FCoeff[x] * lFhbnd[zy*4+n];
          }
        }
      }
      break;
    case 1: // y faces
      for (int z = 0; z < 1; z++) { // z
      for (int x = 0; x < 4; x++) { // x
        const cCompType scaledWeight = scaling * weights2[z*4+x];
        for (int y = 0; y < 4; y++) { // y
          #pragma omp simd aligned(lduh,FCoeff,lFhbnd:16)
          for (int n = 0; n < 4; n++) {
            lduh[((z*4+y)*4+x)*4+n] -= scaledWeight * FCoeff[y] * lFhbnd[(z*4+x)*4+n];
          }
        }
      }
      }
      break;
    case 2: // z faces
    
      for (int yx = 0; yx < 16; yx++) { // yx
        const cCompType scaledWeight = scaling * weights2[yx];
        for (int z = 0; z < 4; z++) { // x
          #pragma omp simd aligned(lduh,FCoeff,lFhbnd:16)
          for (int n = 0; n < 4; n++) {
            lduh[(z*16+yx)*4+n] -= scaledWeight * FCoeff[z] * lFhbnd[yx*4+n];
          }
        }
      }
      break;
  }
}


template void generated::kernels::AderDG::nonlinear::faceIntegral(
  double *lduh, 
  const double *const lFhbnd,
  const int direction, 
  const int orientation,
  const double inverseDxDirection);