// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Partition.h"
#include "peano4/RefinementRequest.h"
#include "tarch/accelerator/CompileTarget.h"

namespace peano4::tests {
  TARGET_DECLARE
  /**
   * The smallest depth whose regular grid keeps coarse cells on both sides of the block getRefinementOfCentreCell()
   * refines.
   */
  constexpr int DepthWithCoarseCellsBesideTheCentreBlock = peano4::SubdivisionFactor >= 3 ? 2 : 3;
  TARGET_DECLARE_END

  void buildRegularGrid(int depth, const std::bitset<DIMENSIONS>& periodicBoundaries = std::bitset<DIMENSIONS>());
  void refineGrid(const std::vector<peano4::RefinementRequest>& requests);

  peano4::RefinementRequest getRefinementOfCentreCell(int depth);
  peano4::RefinementRequest getRefinementOfFirstChildOfRoot(int depth);
  std::vector<peano4::RefinementRequest> getErasureOfEveryCell(int depth);

  int getNumberOfCellsPerAxisRefinedAroundTheCentre();

  int getGlobalSumOf(int localValue);
} // namespace peano4::tests
