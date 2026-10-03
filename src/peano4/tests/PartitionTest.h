// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class PartitionTest;
} // namespace peano4::tests

/**
 * Checks the lists of the partition on regular, refined and periodic grids, across a regrid and in
 * the device twin, and the numbering of the cells along the curve over all ranks.
 */
class peano4::tests::PartitionTest final : public tarch::tests::TestCase {
public:
  PartitionTest();
  PartitionTest(const PartitionTest& other)            = delete;
  PartitionTest(PartitionTest&& other)                 = delete;
  PartitionTest& operator=(const PartitionTest& other) = delete;
  PartitionTest& operator=(PartitionTest&& other)      = delete;
  ~PartitionTest() override                            = default;

  void run() override;

private:
  void testTheRegularGridLinksEveryFaceToItsCells();
  void testThePeriodicGridHasNoBoundaryFaces();
  void testTheRefinedBlockBuildsOneLevelJumpPerCoarseFace();
  void testTheListsGroupByAdjacencyLevelAndKind();
  void testARegridKeepsTheSlotsOfSurvivingCellsAndReusesFreedOnes();
  void testTheExchangedFacesMatchAcrossRanks();
  void testTheCurveIndicesNumberTheCellsAlongTheCurve();
  void testTheDeviceTwinReadsTheSameGeometry();
  void testTheBytesCountEveryArray();
};
