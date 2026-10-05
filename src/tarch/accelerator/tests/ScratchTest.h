// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::tests {
  class ScratchTest;
} // namespace tarch::accelerator::tests

class tarch::accelerator::tests::ScratchTest final : public tarch::tests::TestCase {
public:
  ScratchTest();
  ScratchTest(const ScratchTest&)            = delete;
  ScratchTest(ScratchTest&&)                 = delete;
  ScratchTest& operator=(const ScratchTest&) = delete;
  ScratchTest& operator=(ScratchTest&&)      = delete;
  ~ScratchTest() override                    = default;

  void run() override;

private:
  void testEveryWorkItemNamesAChunkOfTheDispatch();
  void testTheChunksCoverTheTeamOfTheEnclosingRegion();
};
