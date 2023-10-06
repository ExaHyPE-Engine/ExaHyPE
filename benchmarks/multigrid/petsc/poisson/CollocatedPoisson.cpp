#include "CollocatedPoisson.h"


benchmarks::multigrid::petsc::poisson::CollocatedPoisson::CollocatedPoisson() {
  // @todo add your stuff here
}


benchmarks::multigrid::petsc::poisson::CollocatedPoisson::~CollocatedPoisson() {
  // @todo add your stuff here
}


void benchmarks::multigrid::petsc::poisson::CollocatedPoisson::vertexInit(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  
  double&                                       value,
  double&                                       rhs
  
) {
  rhs = Dimensions * tarch::la::PI * tarch::la::PI;
  for (int i = 0; i < Dimensions; i++){

    //we add scaling by cell width here
    rhs *= std::sin( tarch::la::PI * x[i] ) * h[i];
  }
  value = 0.0;
}


