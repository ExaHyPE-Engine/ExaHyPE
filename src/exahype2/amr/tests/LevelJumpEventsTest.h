// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::amr::tests {
  class LevelJumpEventsTest;
} // namespace exahype2::amr::tests

class exahype2::amr::tests::LevelJumpEventsTest final : public tarch::tests::TestCase {
public:
  LevelJumpEventsTest();
  LevelJumpEventsTest(const LevelJumpEventsTest&)            = delete;
  LevelJumpEventsTest(LevelJumpEventsTest&&)                 = delete;
  LevelJumpEventsTest& operator=(const LevelJumpEventsTest&) = delete;
  LevelJumpEventsTest& operator=(LevelJumpEventsTest&&)      = delete;
  ~LevelJumpEventsTest() override                            = default;

  void run() override;

private:
  void testClearTransferBuffer();
  void testClearTransferRecord();
  void testSubfaceIndexWithinFatherFace();
  void testChildIndexWithinFatherCell();
  void testIsOnDomainBoundary();
  void testInterpolateFaceSkipsSubfaceInsideThePatch();
  void testRestrictFaceFromStagedSubfaces();
  void testCollectStagedSubfacesReportsAnAbsentSubface();
  void testRestrictChildrenIntoCellWaitsForTheSiblingSet();
  void testProlongCellIntoChild();
  void testFaceLabelBookkeeping();
  void testCellLabelBookkeeping();
  void testDataTransferFlags();
  void testCellAveragingKernelsAverageTheChildren();
};
