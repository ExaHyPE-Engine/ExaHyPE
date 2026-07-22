// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class ApplySplit1DRiemannToPatchTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::ApplySplit1DRiemannToPatchTest : public tarch::tests::TestCase {
public:
  ApplySplit1DRiemannToPatchTest();

  void run() override;

private:
  void testIterateGrid();
};
