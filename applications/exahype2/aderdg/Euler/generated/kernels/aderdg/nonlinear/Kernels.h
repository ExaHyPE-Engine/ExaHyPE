
#ifndef _EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_KERNELS_H_
#define _EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_KERNELS_H_

#ifndef __INTEL_COMPILER
#include <mm_malloc.h>
#endif
#include <vector>

#include "tarch/la/Vector.h"

//forward declaration of the user solver
namespace examples {
namespace exahype2 {
namespace euler2D {

  class euler2D;
  
}
}
}


namespace generated {
namespace kernels {
namespace AderDG {
namespace nonlinear {
  template <typename cStoreType, typename pCompType, typename pStoreType>
  int fusedSpaceTimePredictorVolumeIntegral(
    examples::exahype2::euler2D::euler2D& solver,
    pCompType* __restrict__ lduh,
    pStoreType* __restrict__ lQhbnd,
    pStoreType* __restrict__ lGradQhbnd,
    pStoreType* __restrict__ lFhbnd,
    pCompType* __restrict__ lQi,
    pCompType* __restrict__ rhs,
    pCompType* __restrict__ lFi,
    pCompType* __restrict__ lSi,   // for NCP or Source
    pCompType* __restrict__ lQhi,
    pCompType* __restrict__ lFhi,
    pCompType* __restrict__ lShi,  // for NCP or Source
    pCompType* __restrict__ gradQ, // for NCP or Source or viscousFlux
    pCompType* __restrict__ gradQAvg, // for viscousFlux
    const cStoreType* const __restrict__ luh,
    const tarch::la::Vector<Dimensions,double>&  x,
    const tarch::la::Vector<Dimensions,double>&  dx,
    const double timeStamp,
    const double dt
  );

  template<typename cStoreType, typename compType>
  void solutionUpdate( 
    cStoreType* __restrict__ luh,
    const cStoreType* __restrict__ const luhOld, 
    const compType* __restrict__ const lduh, 
    const double dt
  );
  
  void surfaceIntegral( 
    double* __restrict__ lduh, 
    const double* __restrict__ const lFhbnd, 
    const double inverseDx //Assume dx[0] == dx[1] == dx[2]
  );
  
  template <typename cCompType>
  void faceIntegral(
    cCompType *lduh, 
    const cCompType *const lFhbnd,
    const int direction, 
    const int orientation,
    const double inverseDxDirection
  );

  void solutionAdjustment(
    examples::exahype2::euler2D::euler2D& solver,
    double* luh,
    const double* const center,
    const double dx, //Assume dx[0] == dx[1] == dx[2]
    const double t,
    const double dt
  );

template <typename pStoreType, typename cCompType>
  void riemannSolver( 
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
  );

  double stableTimeStepSize(
    examples::exahype2::euler2D::euler2D& solver,
    const double* __restrict__ const luh,
    const tarch::la::Vector<Dimensions, double>& cellCentre,
    const tarch::la::Vector<Dimensions, double>& dx,
    const double t,
    const double last_dt
  );

  template <typename pStoreType, typename cCompType>
  void boundaryConditions(
    examples::exahype2::euler2D::euler2D& solver,
    cCompType* fluxOut, 
    pStoreType* stateOut, 
    const cCompType* const fluxIn, 
    const pStoreType* const stateIn, 
    const pStoreType* const gradStateIn, 
    const tarch::la::Vector<Dimensions,double>& cellCentre,
    const tarch::la::Vector<Dimensions,double>& cellSize,
    const double t,const double dt, 
    const int faceIndex, 
    const int normalNonZero 
  );

  
//AMR Routines
//------------

  void faceUnknownsProlongation(
    double* __restrict__ lQhbndFine,
    double* __restrict__ lFhbndFine,
    const double* const __restrict__ lQhbndCoarse,
    const double* const __restrict__ lFhbndCoarse,
    const int coarseGridLevel,
    const int fineGridLevel,
    const int* const subfaceIndex
  );
  
  // used by faceIntegral, only __restrict__ the flux on the face
  void faceFlux__restrict__ion(
    double* __restrict__ lFhbndCoarse,
    const double* const __restrict__ lFhbndFine,
    const int* const subfaceIndex,
    const int levelDelta
  );

  void volumeUnknownsProlongation(
    double* __restrict__ luhFine,
    const double* const __restrict__ luhCoarse,
    const int coarseGridLevel,
    const int fineGridLevel,
    const int* const subcellIndex
  );
  
  void volumeUnknowns__restrict__ion(
    double* __restrict__ luhCoarse,
    const double* const __restrict__ luhFine,
    const int coarseGridLevel,
    const int fineGridLevel,
    const int* const subcellIndex
  );

  
}
}
}
}

#include "./ConfigurationParameters.cpph"
#include "./matrixUtils.cpph"

#endif // _EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_KERNELS_H_