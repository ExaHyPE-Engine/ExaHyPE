
#include <algorithm>

#include "./Kernels.h"
#include "./Quadrature.h"
#include "./DGMatrices.h"

#include "../../../../euler2D.h"

template<typename pStoreType, typename cCompType>
void generated::kernels::AderDG::nonlinear::boundaryConditions(
  examples::exahype2::euler2D::euler2D&                             solver,
  cCompType*                                  fluxOut,
  pStoreType*                                 stateOut,
  const cCompType* const                      fluxIn,
  const pStoreType* const                     stateIn,
  const pStoreType* const                     gradStateIn,
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const tarch::la::Vector<Dimensions,double>& cellSize,
  const double t,const double                 dt,
  const int                                   faceIndex,
  const int                                   normalNonZero
) {                         
  // Compute if face is "left" (0,2,4) or "right" face (1,2,3).
  const int f = faceIndex-2*normalNonZero;

  // Determine the free directions from the non-zero normal entry.
  const int d1 = (3 ^ normalNonZero) / 3; //0->1, 1->0, 2->0 , ^ is bitwise XOR
  
  tarch::la::Vector<Dimensions,double> x;
  x[normalNonZero] = cellCentre[normalNonZero] + (-0.5 + f)*cellSize[normalNonZero];
  
  for (int jj = 0; jj < 1; jj++) {  // loop over dof, loop removed by compiler if 2D
    for (int ii = 0; ii < 4; ii++) {  // loop over dof
      x[d1] = cellCentre[d1] + cellSize[d1] * (nodes[ii] - 0.5); 

#ifdef USE_IPO
    #pragma forceinline recursive
#endif

    solver.examples::exahype2::euler2D::euler2D::boundaryConditions(
      &stateIn[(jj*1+ii)*4],  //Qinside
      &stateOut[(jj*1+ii)*4], //Qoutside
      x,
      cellSize,
      t,
      normalNonZero
    );

    solver.examples::exahype2::euler2D::euler2D::flux(
      &stateOut[(jj*1+ii)*4],
      x,
      cellSize,
      t,
      dt,
      normalNonZero,
      &fluxOut[(jj*1+ii)*4]
    );

//    solver.examples::exahype2::euler2D::euler2D::boundaryValues(x,t,dt,faceIndex,normalNonZero,
//      &fluxIn[(jj*1+ii)*4], &stateIn[(jj*1+ii)*4],
//      &fluxOut[(jj*1+ii)*4],&stateOut[(jj*1+ii)*4]);


    }
  }
  
}

template void generated::kernels::AderDG::nonlinear::boundaryConditions(
  examples::exahype2::euler2D::euler2D& solver,
  double* fluxOut,
  double* stateOut,
  const double* const fluxIn,
  const double* const stateIn,
  const double* const gradStateIn,
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const tarch::la::Vector<Dimensions,double>& cellSize,
  const double t,const double dt,
  const int faceIndex,
  const int normalNonZero);