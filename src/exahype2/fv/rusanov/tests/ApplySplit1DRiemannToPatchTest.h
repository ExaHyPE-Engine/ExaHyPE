// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::rusanov::tests {
  class ApplySplit1DRiemannToPatchTest;
} // namespace exahype2::fv::rusanov::tests

class exahype2::fv::rusanov::tests::ApplySplit1DRiemannToPatchTest: public tarch::tests::TestCase {
private:
  void testIterateGrid();

public:
  ApplySplit1DRiemannToPatchTest();
  virtual ~ApplySplit1DRiemannToPatchTest() override = default;

  virtual void run() override;
};
