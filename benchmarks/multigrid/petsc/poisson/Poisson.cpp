#include "Poisson.h"


benchmarks::multigrid::petsc::poisson::Poisson::Poisson() {
  // previously, this code would segfault without reserving
  // some places for members. 16 is hardcoded.
  // _valueAndRhs.reserve(16);
}


benchmarks::multigrid::petsc::poisson::Poisson::~Poisson() {
  // @todo add your stuff here
}


void benchmarks::multigrid::petsc::poisson::Poisson::init(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  
  double&                                       value,
  double&                                       rhs
  
) {
  value = 0;
  rhs = 1;
}
