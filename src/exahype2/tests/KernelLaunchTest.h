
// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class KernelLaunchTest;
} // namespace exahype2::tests

class exahype2::tests::KernelLaunchTest: public tarch::tests::TestCase {
private:
  void testDimensions();

public:
  KernelLaunchTest();
  virtual ~KernelLaunchTest() override = default;

  virtual void run() override;
};
