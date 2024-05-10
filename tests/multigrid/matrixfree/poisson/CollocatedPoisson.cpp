#include "CollocatedPoisson.h"


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
  double tempRhs = 2 * tarch::la::PI * tarch::la::PI;
  for (int i = 0; i < Dimensions; i++){
    tempRhs *= std::sin( tarch::la::PI * x[i] );
  }
  // put this into the rhs vector
  for (int i=0; i<rhs.size(); i++)
  {
    rhs[i] = tempRhs;
  }

  // similarly, set all of the entries of "value" to 0
  for (int i=0; i<value.size(); i++)
  {
    value[i] = 0.0;
  }
}
