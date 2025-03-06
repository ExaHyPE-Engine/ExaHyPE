#include "DGPoisson.h"
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
#include "Scenario.h"


tests::multigrid::matrixfree::poisson::DGPoisson::DGPoisson() {
}


tests::multigrid::matrixfree::poisson::DGPoisson::~DGPoisson() {
}


//! [Set initial guess and rhs]
void tests::multigrid::matrixfree::poisson::DGPoisson::initCellDoF(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< UnknownsPerCellNode, double >&    value,
  tarch::la::Vector< UnknownsPerCellNode, double >&    rhs,
  tarch::la::Vector< Dimensions, int >&         node
) {
  logTraceInWith4Arguments("initCellDoF(...)", x, h, value, rhs);

  rhs[0]   = getPointWiseRhs(x);
  value[0] = 0;

  logTraceOutWith4Arguments("initCellDoF(...)", x, h, value, rhs);
}
//! [Set initial guess and rhs]

