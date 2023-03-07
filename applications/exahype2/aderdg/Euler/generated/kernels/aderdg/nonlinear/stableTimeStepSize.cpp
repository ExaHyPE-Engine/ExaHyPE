
#include <limits>

#include "./Kernels.h"

#include "../../../../euler2D.h"

#include "exahype2/dg/DGUtils.h"
#include "peano4/utils/Loop.h"

double generated::kernels::AderDG::nonlinear::stableTimeStepSize(
  examples::exahype2::euler2D::euler2D& solver,
  const double* __restrict__ const luh,
  const tarch::la::Vector<Dimensions, double>& cellCentre,
  const tarch::la::Vector<Dimensions, double>& dx,
  const double t,
  const double last_dt) {
  constexpr double cflFactor = examples::exahype2::euler2D::euler2D::CFL;
  constexpr double PNPM      = examples::exahype2::euler2D::euler2D::PNPM;
  const double inverseDx = 1/dx[0]; //Assume dx[0] == dx[1] == dx[2]
  
  double dt = std::numeric_limits<double>::max();
  
  dfor(index, 4){

    int linearisedIndex = 0;
    int stride = 1;
    for(int d=0; d<Dimensions; d++){        
      linearisedIndex += stride*index[d];
      stride *= 4;
    }

    double denominator = 0.0;
    for(int d=0; d<Dimensions; d++){
      double maxEigenvalue = 
        solver.maxEigenvalue(
          &luh[linearisedIndex*4],

          ::exahype2::dg::getQuadraturePoint(
            cellCentre,
            dx,
            index, //index
            3,
            solver.QuadraturePoints1d
          ), //faceCentre
          dx,
          t,
          last_dt,
          d
        );
      denominator += maxEigenvalue * inverseDx; //Assume dx[0] == dx[1] == dx[2]

      }

      dt = std::min(dt, cflFactor * PNPM / denominator);

  }

  return dt;
}