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
  //divide by 2 to achieve correct scaling
  // @todo TW: I'm not sure this dividion is correct
  rhs = Dimensions * tarch::la::PI * tarch::la::PI / 2;
  for (int i = 0; i < Dimensions; i++){

    //we add scaling by cell width here
    rhs *= std::sin( tarch::la::PI * x[i] ) * h[i];
  }
  value = 0.0;
}














