#include "DGPoisson.h"


benchmarks::multigrid::matrixfree::poisson::DGPoisson::DGPoisson() {
}


benchmarks::multigrid::matrixfree::poisson::DGPoisson::~DGPoisson() {
}

void benchmarks::multigrid::matrixfree::poisson::DGPoisson::initVertex(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< VertexUnknowns, double >&  value,
  tarch::la::Vector< VertexUnknowns, double >&  rhs
) { return; }

void benchmarks::multigrid::matrixfree::poisson::DGPoisson::initCell(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< CellUnknowns, double >&  value,
  tarch::la::Vector< CellUnknowns, double >&  rhs
) {return;}

void benchmarks::multigrid::matrixfree::poisson::DGPoisson::initFace(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< FaceUnknownsSolution,   double >&  solution,
  tarch::la::Vector< FaceUnknownsProjection, double >&  projection
) {return;}