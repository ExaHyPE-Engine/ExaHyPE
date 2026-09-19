// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::amr::tests {
  class ChildDataStageTest;
} // namespace exahype2::amr::tests

class exahype2::amr::tests::ChildDataStageTest final : public tarch::tests::TestCase {
public:
  ChildDataStageTest();
  ChildDataStageTest(const ChildDataStageTest&)            = delete;
  ChildDataStageTest(ChildDataStageTest&&)                 = delete;
  ChildDataStageTest& operator=(const ChildDataStageTest&) = delete;
  ChildDataStageTest& operator=(ChildDataStageTest&&)      = delete;
  ~ChildDataStageTest() override                           = default;

  void run() override;

private:
  void testSiblingSetLifecycle();
  void testNestedFrames();
  void testPartialFrameAccessors();
  void testClaimedFramesCarryTheirTag();
};
