// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::parallel::tests {
  class RunControlTest;
} // namespace peano4::parallel::tests

class peano4::parallel::tests::RunControlTest final : public tarch::tests::TestCase {
public:
  RunControlTest();
  void run() override;

private:
  void testProgramStepBroadcast();
  void testTerminationBroadcast();
};
