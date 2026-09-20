// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class ApplySplit1DRiemannToPatchTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::ApplySplit1DRiemannToPatchTest final : public tarch::tests::TestCase {
public:
  ApplySplit1DRiemannToPatchTest();
  ApplySplit1DRiemannToPatchTest(const ApplySplit1DRiemannToPatchTest&)            = delete;
  ApplySplit1DRiemannToPatchTest(ApplySplit1DRiemannToPatchTest&&)                 = delete;
  ApplySplit1DRiemannToPatchTest& operator=(const ApplySplit1DRiemannToPatchTest&) = delete;
  ApplySplit1DRiemannToPatchTest& operator=(ApplySplit1DRiemannToPatchTest&&)      = delete;
  ~ApplySplit1DRiemannToPatchTest() override                                       = default;

  void run() override;

private:
  void testSplitTraversalMatchesReference();
};
