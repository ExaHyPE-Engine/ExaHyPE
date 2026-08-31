// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::blockstructured::tests {
  class ChildDataStageTest;
} // namespace toolbox::blockstructured::tests

class toolbox::blockstructured::tests::ChildDataStageTest final : public tarch::tests::TestCase {
public:
  ChildDataStageTest();
  ~ChildDataStageTest() override = default;

  void run() override;

private:
  void testSiblingSetLifecycle();
  void testNestedFrames();
  void testPartialFrameAccessors();
  void testClaimedFramesCarryTheirTag();
};
