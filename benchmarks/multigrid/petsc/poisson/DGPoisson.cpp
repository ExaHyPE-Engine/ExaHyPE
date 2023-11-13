#include "DGPoisson.h"


benchmarks::multigrid::petsc::poisson::DGPoisson::DGPoisson() {
  // @todo add your stuff here
}


benchmarks::multigrid::petsc::poisson::DGPoisson::~DGPoisson() {
  // @todo add your stuff here
}


void benchmarks::multigrid::petsc::poisson::DGPoisson::initNode(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  
  double&                                       value,
  double&                                       rhs
  
) {
  logTraceInWith4Arguments("initNode", x, h, value, rhs);
  rhs = Dimensions * tarch::la::PI * tarch::la::PI;
  for (int i = 0; i < Dimensions; i++){
    rhs *= std::sin( tarch::la::PI * x[i] );
  }
  value = 0.0;
  logTraceOutWith4Arguments("initNode", x, h, value, rhs);
}














