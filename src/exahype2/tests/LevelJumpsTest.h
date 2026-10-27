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
  LevelJumpsTest(const LevelJumpsTest& other)            = delete;
  LevelJumpsTest(LevelJumpsTest&& other)                 = delete;
  LevelJumpsTest& operator=(const LevelJumpsTest& other) = delete;
  LevelJumpsTest& operator=(LevelJumpsTest&& other)      = delete;
  ~LevelJumpsTest() override                             = default;

  void run() override;

private:
  void testContainsIntermediateFaceReturnsFalseOnEveryLevelOfTheRefinedGrid();
  void testTheInterpolationsVisitEveryLevelJumpOnceAfterTheWritesOfItsCoarseFace();
  void testTheRestrictionVisitsEveryLevelJumpOnceAfterTheWritesOfItsFineFaces();
  void testTheGroupedRestrictionsVisitEveryLevelJumpOnceAfterTheWritesOfItsFineFaces();
};
