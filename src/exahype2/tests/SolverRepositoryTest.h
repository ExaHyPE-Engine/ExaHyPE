// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class SolverRepositoryTest;
} // namespace exahype2::tests

class exahype2::tests::SolverRepositoryTest final : public tarch::tests::TestCase {
public:
  SolverRepositoryTest();
  SolverRepositoryTest(const SolverRepositoryTest&)            = delete;
  SolverRepositoryTest(SolverRepositoryTest&&)                 = delete;
  SolverRepositoryTest& operator=(const SolverRepositoryTest&) = delete;
  SolverRepositoryTest& operator=(SolverRepositoryTest&&)      = delete;
  ~SolverRepositoryTest() override                             = default;

  void run() override;

private:
  void testTheRepositoryReducesTheValuesOfEverySolver();
  void testAPlotNeedsEverySolverAndASweepFlagNeedsOne();
  void testEverySweepCallReachesEverySolver();
};
