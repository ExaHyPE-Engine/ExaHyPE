#include "DGPoissonRediscretise.h"


benchmarks::mghype::poisson::DGPoissonRediscretise::DGPoissonRediscretise() {
}


benchmarks::mghype::poisson::DGPoissonRediscretise::~DGPoissonRediscretise() {
}



void benchmarks::mghype::poisson::DGPoissonRediscretise::initCellDoF(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< UnknownsPerCellNode, double >&    value,
  tarch::la::Vector< UnknownsPerCellNode, double >&    rhs,
  tarch::la::Vector< Dimensions, int >&         node
) {
  logTraceInWith4Arguments("initCellDoF(...)", x, h, value, rhs);

  rhs[0]   = ::tests::multigrid::matrixfree::poisson::getPointWiseRhs(x);
  value[0] = 0;

  logTraceOutWith4Arguments("initCellDoF(...)", x, h, value, rhs);
}
