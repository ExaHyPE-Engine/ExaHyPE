// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class RollOverUpdatesTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::RollOverUpdatesTest final : public tarch::tests::TestCase {
public:
  RollOverUpdatesTest();
  RollOverUpdatesTest(const RollOverUpdatesTest&)            = delete;
  RollOverUpdatesTest(RollOverUpdatesTest&&)                 = delete;
  RollOverUpdatesTest& operator=(const RollOverUpdatesTest&) = delete;
  RollOverUpdatesTest& operator=(RollOverUpdatesTest&&)      = delete;
  ~RollOverUpdatesTest() override                            = default;

  void run() override;

private:
  void testUpdatedHalfIsCopiedAndBothHalvesMoveTheirTimeStamps();
  void testHalfWithoutATransferRollsOverAfterATopologyChange();
  void testCellSolutionSwapsOnlyAfterAnUpdate();
};
