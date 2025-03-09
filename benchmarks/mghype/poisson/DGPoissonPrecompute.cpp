#include "DGPoissonPrecompute.h"

#include "../../../tests/multigrid/matrixfree/poisson/Scenario.h"

#include "tarch/la/LUDecomposition.h"


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
  logTraceInWith4Arguments("initCellDoF(...)", x, h, value, rhs);

  rhs[0]   = ::tests::multigrid::matrixfree::poisson::getPointWiseRhs(x);
  value[0] = 0;

  logTraceOutWith4Arguments("initCellDoF(...)", x, h, value, rhs);
}


tarch::la::Matrix< benchmarks::mghype::poisson::DGPoissonPrecompute::CellUnknowns, benchmarks::mghype::poisson::AbstractDGPoissonPrecompute::CellUnknowns, double > benchmarks::mghype::poisson::DGPoissonPrecompute::getInvertedApproxSystemMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Vector<Dimensions, double>  unitCellCentre(0.0);
  static tarch::la::Vector<Dimensions, double>  unitCellSize(1.0);

  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > resultOnReferenceCell = AbstractDGPoissonPrecompute::getInvertedApproxSystemMatrix(
    unitCellCentre,
    unitCellSize
  );

  #if Dimensions==2
  return resultOnReferenceCell;
  #else
  return 1.0/cellSize(0) * resultOnReferenceCell;
  #endif
}

