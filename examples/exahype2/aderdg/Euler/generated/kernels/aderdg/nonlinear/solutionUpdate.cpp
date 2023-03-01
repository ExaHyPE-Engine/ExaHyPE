
// update the elements 

#include "./Kernels.h"
#include "./Quadrature.h"

template<typename cStoreType, typename compType>
void generated::kernels::AderDG::nonlinear::solutionUpdate( 
  cStoreType* __restrict__ luh,
  const cStoreType* __restrict__ const luhOld,
  const compType* __restrict__ const lduh, 
  const double dt
) {
#ifdef __INTEL_COMPILER
  __assume_aligned(iweights3, 16);
  __assume_aligned(luh,       16); //luh    should be aligned, see Solver.h
  __assume_aligned(luhOld,    16); //luhOld should be aligned, see Solver.h
  __assume_aligned(lduh,      16); //lduh   should be aligned, see Solver.h
#endif

  for (int xyz = 0; xyz < 16; xyz++) {
    const compType coeff = dt*iweights3[xyz];
    #pragma omp simd aligned(luh,luhOld,lduh:16)
    for (int n = 0; n < 4; n++) { //update only the variables, lduh contains no parameters
      luh[xyz*4+n] = luhOld[xyz*4+n] + coeff*lduh[xyz*4+n]; //simd+fma
    }
  }
 
}

template void generated::kernels::AderDG::nonlinear::solutionUpdate( 
  double* __restrict__ luh,
  const double* __restrict__ const luhOld,
  const double* __restrict__ const lduh, 
  const double dt);

