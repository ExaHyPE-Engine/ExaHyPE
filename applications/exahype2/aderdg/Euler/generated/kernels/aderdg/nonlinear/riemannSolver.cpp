
#include <algorithm>
#include <cstring>
#include <cmath>

#include "./Kernels.h"
#include "./DGMatrices.h"
#include "./Quadrature.h"

#include "../../../../euler2D.h"

template <typename pStoreType, typename cCompType>
void generated::kernels::AderDG::nonlinear::riemannSolver( 
  examples::exahype2::euler2D::euler2D& solver,
  cCompType* __restrict__ FL, 
  cCompType* __restrict__ FR,
  const pStoreType* __restrict__ const QL, 
  const pStoreType* __restrict__ const QR,
  const double t,
  const double dt,
  const tarch::la::Vector<Dimensions,double>& faceCentre,
  const tarch::la::Vector<Dimensions,double>& dx,
  const int direction
) {

#ifdef __INTEL_COMPILER
  __assume_aligned(weights2, 16);
  __assume_aligned(FL, 16);
  __assume_aligned(FR, 16);
  __assume_aligned(QL, 16);
  __assume_aligned(QR, 16);
#endif

  double* tmp_bnd = ((double *) _mm_malloc(sizeof(double)*16, 16));
  cCompType QavL[4] __attribute__((aligned(16))) = {0.0};
  cCompType QavR[4] __attribute__((aligned(16))) = {0.0};
  
  for (int xy = 0; xy < 4; xy++) { //xy = 1 or 2 spatial dim (1 in 2D, 2 in 3D)
    #pragma omp simd aligned(QavL,QavR,QL,QR:16)
    for (int n = 0; n < 4; n++) {
      QavL[n] += weights2[xy] * QL[xy*4+n];
      QavR[n] += weights2[xy] * QR[xy*4+n];
    }
  }
  
  // Check hyperbolic eigenvalues.
#ifdef USE_IPO
  #pragma forceinline recursive
#endif
  cCompType smaxL = solver.examples::exahype2::euler2D::euler2D::maxEigenvalue(&QavL[0], faceCentre, dx, t, dt, direction);
#ifdef USE_IPO
  #pragma forceinline recursive
#endif
  cCompType smaxR = solver.examples::exahype2::euler2D::euler2D::maxEigenvalue(&QavR[0], faceCentre, dx, t, dt, direction);
  cCompType smax = std::max(smaxL, smaxR);
  
 
  
  
  for (int xy = 0; xy < 4; xy++){
    #pragma omp simd aligned(tmp_bnd,QL,QR:16)
    for (int n = 0; n < 4; n++) { //skip parameters
      tmp_bnd[xy*4+n] = smax * (QL[xy*4+n]-QR[xy*4+n]);
    }
  }
  
  #pragma omp simd aligned(FL,FR,tmp_bnd:16)
  for (int xyn = 0; xyn < 16; xyn++) {
    FL[xyn] = 0.5 * (FL[xyn] + FR[xyn] + tmp_bnd[xyn]);
  }
  std::copy_n(FL, 16, FR);
  _mm_free(tmp_bnd);
  

}

template void generated::kernels::AderDG::nonlinear::riemannSolver(
  examples::exahype2::euler2D::euler2D& solver,
  double* __restrict__ FL, 
  double* __restrict__ FR,
  const double* const __restrict__ QL, 
  const double* const __restrict__ QR,
  const double t,
  const double dt,
  const tarch::la::Vector<Dimensions,double>& faceCentre,
  const tarch::la::Vector<Dimensions,double>& dx,
  const int direction);