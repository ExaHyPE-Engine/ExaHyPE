#include "DGPoissonPrecompute.h"


benchmarks::mghype::poisson::DGPoissonPrecompute::DGPoissonPrecompute() {
}


benchmarks::mghype::poisson::DGPoissonPrecompute::~DGPoissonPrecompute() {
}



void benchmarks::mghype::poisson::DGPoissonPrecompute::initCellDoF(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< UnknownsPerCellNode, double >&    value,
  tarch::la::Vector< UnknownsPerCellNode, double >&    rhs,
  tarch::la::Vector< Dimensions, int >&         node
) {
  // @todo init your code here
}
