// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class LevelJumpsTest;
} // namespace exahype2::tests

class exahype2::tests::LevelJumpsTest final : public tarch::tests::TestCase {
public:
  LevelJumpsTest();
  LevelJumpsTest(const LevelJumpsTest&)            = delete;
  LevelJumpsTest(LevelJumpsTest&&)                 = delete;
  LevelJumpsTest& operator=(const LevelJumpsTest&) = delete;
  LevelJumpsTest& operator=(LevelJumpsTest&&)      = delete;
  ~LevelJumpsTest() override                       = default;

  void run() override;

private:
  void testTheInterpolationsVisitEveryLevelJumpOnceCoarsestLevelFirst();
  void testTheRestrictionVisitsEveryLevelJumpOnceFinestLevelFirst();
  void testTheGroupedRestrictionsVisitEveryLevelJumpOnceFinestLevelFirst();
};
