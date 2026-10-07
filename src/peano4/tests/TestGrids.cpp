// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestGrids.h"

#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/topology/Tree.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

namespace {
  const tarch::la::Vector<DIMENSIONS, double> DomainOffset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> DomainWidth(1.0);
} // namespace

/**
 * Builds the regular grid of depth on the unit domain over all ranks.
 */
void
peano4::tests::buildRegularGrid(const int depth, const std::bitset<DIMENSIONS>& periodicBoundaries) {
  auto& spacetree = peano4::Spacetree::getInstance();
  spacetree.shutdown();
  spacetree.init(DomainOffset, DomainWidth, periodicBoundaries);
  spacetree.scheduleRegularGridConstruction(depth);
  peano4::Regrid regrid;
  if (not spacetree.regrid({}, regrid)) {
    tarch::terminateWithError("The construction of the regular test grid must change the grid.");
  }
}

/**
 * Applies the requests to the current grid without live data. The partition gets new slot numbers.
 */
void
peano4::tests::refineGrid(const std::vector<peano4::RefinementRequest>& requests) {
  peano4::Regrid regrid;
  if (not peano4::Spacetree::getInstance().regrid(requests, regrid)) {
    tarch::terminateWithError("The refinement of the test grid must change the grid.");
  }
}

/**
 * Returns the request that refines the centre cell of the base grid of depth. The refined block
 * spans getNumberOfCellsPerAxisRefinedAroundTheCentre() cells per axis.
 */
peano4::RefinementRequest
peano4::tests::getRefinementOfCentreCell(const int depth) {
  peano4::RefinementRequest request;
  request.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
  request.level        = depth;
  request.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(peano4::topology::getCellsPerAxis(depth) / 2);
  return request;
}

/**
 * Returns the request that refines exactly the cells of the base grid of depth inside the first
 * child of the root. Every face between this child and its neighbours becomes a level jump.
 */
peano4::RefinementRequest
peano4::tests::getRefinementOfFirstChildOfRoot(const int depth) {
  const int cellsPerAxisOfChild          = peano4::topology::getCellsPerAxis(depth) / peano4::SubdivisionFactor;
  const int refinedCellsBesideTheRequest = (getNumberOfCellsPerAxisRefinedAroundTheCentre() - 1) / 2;
  assertion(cellsPerAxisOfChild > refinedCellsBesideTheRequest, depth, cellsPerAxisOfChild);
  assertion(cellsPerAxisOfChild <= 2 * refinedCellsBesideTheRequest + 1, depth, cellsPerAxisOfChild);
  peano4::RefinementRequest request;
  request.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
  request.level        = depth;
  request.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(cellsPerAxisOfChild - 1 - refinedCellsBesideTheRequest);
  return request;
}

/**
 * Returns one erase request per cell of the base grid of the depth. Every refined block coarsens.
 */
std::vector<peano4::RefinementRequest>
peano4::tests::getErasureOfEveryCell(const int depth) {
  const int                              cellsPerAxis = peano4::topology::getCellsPerAxis(depth);
  std::vector<peano4::RefinementRequest> requests;
  for (int cell = 0; cell < peano4::power(cellsPerAxis, DIMENSIONS); cell++) {
    peano4::RefinementRequest request;
    request.setRefinementControl(peano4::RefinementRequest::RefinementControl::Erase);
    request.level        = depth;
    request.latticeIndex = peano4::dDelinearised(cell, cellsPerAxis);
    requests.push_back(request);
  }
  return requests;
}

/**
 * Returns the refined cells per axis: The refined cell, one neighbour on either side and the ring
 * the flagged corner vertices add.
 */
int
peano4::tests::getNumberOfCellsPerAxisRefinedAroundTheCentre() {
  return 5;
}

int
peano4::tests::getGlobalSumOf(const int localValue) {
  int globalValue = 0;
  tarch::mpi::allreduce(
    &localValue,
    &globalValue,
    1,
    MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  return globalValue;
}
