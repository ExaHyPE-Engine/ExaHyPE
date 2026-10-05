// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::runner::tests {
  class StatisticsTest;
} // namespace exahype2::runner::tests

class exahype2::runner::tests::StatisticsTest final : public tarch::tests::TestCase {
public:
  StatisticsTest();
  StatisticsTest(const StatisticsTest& other)            = delete;
  StatisticsTest(StatisticsTest&& other)                 = delete;
  StatisticsTest& operator=(const StatisticsTest& other) = delete;
  StatisticsTest& operator=(StatisticsTest&& other)      = delete;
  ~StatisticsTest() override                             = default;

  void run() override;

private:
  struct PlainSolver;
  struct LimitedSolver;
  struct UnlimitedSolver;

  void testTheGlobalViewSumsAndBoundsTheRankStatistics();
  void testSnapshotFillsTheMemoryRowsOfTheBuild();
  void testDescribeGridSnapshotRendersFilledRows();
  void testDescribeGridTransitionMarksChangedValuesOnly();
  void testResidentAndFreeMemoryFormOneLine();
  void testValueRangeAndCountCarryTheirKind();
  void testASolverWithoutEigenvalueReportsStepAndMeshWidth();
  void testASolverWithEigenvalueAndTroubledCellsAppendsBoth();
  void testASolverWithoutLimiterReportsNoTroubledCells();
  void testAdditionalStatisticsFollowTheMeshWidth();
};
