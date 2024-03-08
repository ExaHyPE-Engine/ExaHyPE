#include "DGPoisson.h"
#include "peano4/utils/Loop.h"

tests::multigrid::matrixfree::poisson::DGPoisson::DGPoisson() {
}


tests::multigrid::matrixfree::poisson::DGPoisson::~DGPoisson() {
}

void tests::multigrid::matrixfree::poisson::DGPoisson::initVertex(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< VertexUnknowns, double >&  value,
  tarch::la::Vector< VertexUnknowns, double >&  rhs
) { return; }

//! [Set initial guess and rhs]
void tests::multigrid::matrixfree::poisson::DGPoisson::initCell(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< CellUnknowns, double >&  value,
  tarch::la::Vector< CellUnknowns, double >&  rhs,
  tarch::la::Vector< CellUnknowns, double >&  exact
) {
  
  logTraceInWith5Arguments("initCell", x, h, value, rhs, exact);

  auto bottomLeft = x - 0.5 * h;

  static constexpr double fourDPiSquared = 4 * Dimensions * tarch::la::PI * tarch::la::PI;
  
  // we only have one cell unknown, so we can reuse this macro
  int counter = 0;
  dfor( k, PolyDegree + 1 )
  {
    // convert k to double
    tarch::la::Vector<Dimensions, double> kDouble = tarch::la::convertScalar<double, Dimensions, int>(k);
    auto dofPosition = bottomLeft + h(0)*kDouble;
    double sinSquared = 1;
    for (int i=0; i<Dimensions; i++)
      sinSquared *= std::sin( 2 * tarch::la::PI * dofPosition[i] );
    rhs[counter]   = fourDPiSquared * sinSquared;
    value[counter] = 0;
    exact[counter] = sinSquared;
    counter++;
  }

  logTraceOutWith5Arguments("initCell", x, h, value, rhs, exact);
}
//! [Set initial guess and rhs]

void tests::multigrid::matrixfree::poisson::DGPoisson::initFace(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< FaceUnknownsSolution,   double >&  solution,
  tarch::la::Vector< FaceUnknownsProjection, double >&  projection
) {
  for (int i=0; i < solution.size(); i++)   solution[i]   = 0;
  for (int i=0; i < projection.size(); i++) projection[i] = 0;
}

bool tests::multigrid::matrixfree::poisson::DGPoisson::terminationCriterionHolds()
{
  return std::abs( currentMaxAbs ) < SolverTolerance;
}

/**
* Used for updating mid-sweep
*/
void tests::multigrid::matrixfree::poisson::DGPoisson::updateMaxAbs(double maxAbs)
{
  maxAbsMtx.lock();
  currentMaxAbs = std::max( currentMaxAbs, maxAbs );
  maxAbsMtx.unlock();
}

/**
* Used for actually resetting at start of sweep
*/
void tests::multigrid::matrixfree::poisson::DGPoisson::setMaxAbs(double maxAbs)
{
  maxAbsMtx.lock();
  currentMaxAbs = maxAbs;
  maxAbsMtx.unlock();
}
