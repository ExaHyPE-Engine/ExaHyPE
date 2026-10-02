// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class PartitionTest;
} // namespace peano4::tests

/**
 * Checks that the partition links every face to its cells, groups its lists by dependence, level
 * and kind, builds the level jumps of a refined block, keeps the slots of surviving cells across a
 * regrid, derives exchanged faces both ranks agree on, numbers the cells along the curve over all
 * ranks and reads the same on the device.
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
  void testTheListsGroupByDependenceLevelAndKind();
  void testARegridKeepsTheSlotsOfSurvivingCellsAndReusesFreedOnes();
  void testTheExchangedFacesMatchAcrossRanks();
  void testTheCurveIndicesNumberTheCellsAlongTheCurve();
  void testTheDeviceTwinReadsTheSameGeometry();
  void testTheBytesCountEveryArray();
};
