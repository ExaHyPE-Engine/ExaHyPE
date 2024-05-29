#include "CollocatedPoisson.h"

#include "Scenario.h"


tests::multigrid::matrixfree::poisson::CollocatedPoisson::CollocatedPoisson() {
}


tests::multigrid::matrixfree::poisson::CollocatedPoisson::~CollocatedPoisson() {
}


void tests::multigrid::matrixfree::poisson::CollocatedPoisson::initVertex(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< VertexUnknowns, double >&  value,
  tarch::la::Vector< VertexUnknowns, double >&  rhs
) {
  for (int i=0; i<rhs.size(); i++) {
    rhs(i) = getPointWiseRhs(x);
  }

  // similarly, set all of the entries of "value" to 0
  for (int i=0; i<value.size(); i++)
  {
    value[i] = 0.0;
  }
}


void tests::multigrid::matrixfree::poisson::CollocatedPoisson::setBoundaryConditions(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< VertexUnknowns, double >&  value
) {
  for (int i=0; i<value.size(); i++) {
    value[i] = 0.0;
  }
}
