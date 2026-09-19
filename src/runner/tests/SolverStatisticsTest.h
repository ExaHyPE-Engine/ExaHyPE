// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace runner::tests {
  class SolverStatisticsTest;
} // namespace runner::tests

class runner::tests::SolverStatisticsTest final : public tarch::tests::TestCase {
public:
  SolverStatisticsTest();
  SolverStatisticsTest(const SolverStatisticsTest&)            = delete;
  SolverStatisticsTest(SolverStatisticsTest&&)                 = delete;
  SolverStatisticsTest& operator=(const SolverStatisticsTest&) = delete;
  SolverStatisticsTest& operator=(SolverStatisticsTest&&)      = delete;
  ~SolverStatisticsTest() override                             = default;

  void run() override;

private:
  void testValueRangeAndCountCarryTheirKind();
  void testASolverWithoutEigenvalueReportsStepAndMeshWidth();
  void testASolverWithEigenvalueAndTroubledCellsAppendsBoth();
  void testAdditionalStatisticsFollowTheMeshWidth();
  void testParticleStatisticsCountTheParticles();
};
