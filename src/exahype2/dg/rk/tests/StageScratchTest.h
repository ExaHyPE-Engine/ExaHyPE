// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::rk::tests {
  class StageScratchTest;
} // namespace exahype2::dg::rk::tests

class exahype2::dg::rk::tests::StageScratchTest final : public tarch::tests::TestCase {
public:
  StageScratchTest();
  StageScratchTest(const StageScratchTest&)            = delete;
  StageScratchTest(StageScratchTest&&)                 = delete;
  StageScratchTest& operator=(const StageScratchTest&) = delete;
  StageScratchTest& operator=(StageScratchTest&&)      = delete;
  ~StageScratchTest() override                         = default;

  void run() override;

private:
  void testEveryCallOfOneThreadReturnsTheSameBuffer();
  void testTheBufferHoldsEveryValueOfACell();
};
