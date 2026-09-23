// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class RefinementControlTest;
} // namespace exahype2::tests

class exahype2::tests::RefinementControlTest final : public tarch::tests::TestCase {
public:
  RefinementControlTest();
  RefinementControlTest(const RefinementControlTest&)            = delete;
  RefinementControlTest(RefinementControlTest&&)                 = delete;
  RefinementControlTest& operator=(const RefinementControlTest&) = delete;
  RefinementControlTest& operator=(RefinementControlTest&&)      = delete;
  ~RefinementControlTest() override                              = default;

  void run() override;

private:
  void testDefaultRefinementCommandIsErase();
  void testRefineAndEraseCommandsAreCollected();
  void testKeepCommandIsNotCollected();
  void testClearDropsCollectedCommands();
};
