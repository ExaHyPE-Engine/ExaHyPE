#include "DGPoisson.h"
#include "peano4/utils/Loop.h"

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
) {
  
  logTraceInWith4Arguments("initCell", x, h, value, rhs);

  auto bottomLeft = x - 0.5 * h;

  static constexpr double fourDPiSquared = 4 * Dimensions * tarch::la::PI * tarch::la::PI;
  
  // we only have one cell unknown, so we can reuse this macro
  int counter = 0;
  dfor( k, PolyDegree + 1 )
  {
    // convert k to double
    tarch::la::Vector<Dimensions, double> kDouble = tarch::la::convertScalar<double, Dimensions, int>(k);
    auto dofPosition = bottomLeft + h(0)*kDouble;
    double rhsVal = fourDPiSquared;
    for (int i=0; i<Dimensions; i++)
      rhsVal *= std::sin( 2 * tarch::la::PI * dofPosition[i] );
    rhs[counter]   = rhsVal;
    value[counter] = 0;
    counter++;
  }

  logTraceOutWith4Arguments("initCell", x, h, value, rhs);
}

void benchmarks::multigrid::matrixfree::poisson::DGPoisson::initFace(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< FaceUnknownsSolution,   double >&  solution,
  tarch::la::Vector< FaceUnknownsProjection, double >&  projection
) {
  for (int i=0; i < solution.size(); i++)   solution[i]   = 0;
  for (int i=0; i < projection.size(); i++) projection[i] = 0;
}