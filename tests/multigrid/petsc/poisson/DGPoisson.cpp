#include "DGPoisson.h"


benchmarks::multigrid::petsc::poisson::DGPoisson::DGPoisson() {
  // @todo add your stuff here
}


benchmarks::multigrid::petsc::poisson::DGPoisson::~DGPoisson() {
  // @todo add your stuff here
}


void benchmarks::multigrid::petsc::poisson::DGPoisson::initNode(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  
  double&                                       value,
  double&                                       rhs,
  double&                                       exactSol
  
) {
  logTraceInWith4Arguments("initNode", x, h, value, rhs);
  rhs = 4.0 * Dimensions * tarch::la::PI * tarch::la::PI;
  exactSol = 1.0;
  for (int i = 0; i < Dimensions; i++){
    rhs *= std::sin( 2.0 * tarch::la::PI * x[i] );
    exactSol *= std::sin( 2.0 * tarch::la::PI * x[i] );
  }
  logTraceOutWith4Arguments("initNode", x, h, value, rhs);
}

tarch::la::Matrix< benchmarks::multigrid::petsc::poisson::AbstractDGPoisson::NodesPerFace*benchmarks::multigrid::petsc::poisson::AbstractDGPoisson::FaceUnknowns*benchmarks::multigrid::petsc::poisson::AbstractDGPoisson::FacesPerCell, benchmarks::multigrid::petsc::poisson::AbstractDGPoisson::DoFsPerCell, double > benchmarks::multigrid::petsc::poisson::DGPoisson::getProjectionOfCellDataOntoFace(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
)
{
  // use base class method first
  auto result = AbstractDGPoisson::getProjectionOfCellDataOntoFace(cellCentre, cellSize);

  logTraceInWith3Arguments("getProjectionOfCellDataOntoFace", cellCentre, cellSize, result);

  // just do it the stupid way
  int rows = result.rows();
  int cols = result.cols();
  
  /*
  scale rows 0,1, 4,5, 8,9, 12,13 by 1/h
  */
  for (int i=0; i<14; i+=4)
  for (int j=0; j<cols; j++)
  {
    result(i  ,j) *= 1.0/cellSize(0);
    result(i+1,j) *= 1.0/cellSize(0);
  }

  logTraceInWith1Argument("getProjectionOfCellDataOntoFace", result);
  
  return result;
}


tarch::la::Matrix< benchmarks::multigrid::petsc::poisson::AbstractDGPoisson::DoFsPerCell, 
  benchmarks::multigrid::petsc::poisson::AbstractDGPoisson::NodesPerFace*benchmarks::multigrid::petsc::poisson::AbstractDGPoisson::FaceUnknowns*benchmarks::multigrid::petsc::poisson::AbstractDGPoisson::FacesPerCell,
  double>
  benchmarks::multigrid::petsc::poisson::DGPoisson::getProjectionOfRiemannSolutionOntoCell(
    const tarch::la::Vector<Dimensions, double>&  cellCentre,
    const tarch::la::Vector<Dimensions, double>&  cellSize
  )
  {
  auto result = AbstractDGPoisson::getProjectionOfRiemannSolutionOntoCell(cellCentre, cellSize);
  logTraceInWith3Arguments("getProjectionOfRiemannSolutionOntoCell", cellCentre, cellSize, result);

  // just do it the stupid way
  int rows = result.rows();
  int cols = result.cols();

  // same pattern as above but this time for rows instead of columns.
  for (int j=0; j<14; j+=4)  
  for (int i=0; i<rows; i++)
  {
    result(i,j)   *= cellSize(0);
    result(i,j+1) *= cellSize(0);
  }
  logTraceOutWith1Argument("getProjectionOfRiemannSolutionOntoCell", result);

  /*
  Warning (please investigate later) - not having return statement here
  doesn't throw an error and caller will be in possesion of junk data!
  */
  return result;
}

