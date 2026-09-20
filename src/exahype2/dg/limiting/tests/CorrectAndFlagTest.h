// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class CorrectAndFlagTest;
} // namespace exahype2::dg::limiting::tests

/**
 * Checks the admissibility verdict of one cell and the state it leaves on its labels.
 */
class exahype2::dg::limiting::tests::CorrectAndFlagTest final : public tarch::tests::TestCase {
public:
  CorrectAndFlagTest();
  CorrectAndFlagTest(const CorrectAndFlagTest&)            = delete;
  CorrectAndFlagTest(CorrectAndFlagTest&&)                 = delete;
  CorrectAndFlagTest& operator=(const CorrectAndFlagTest&) = delete;
  CorrectAndFlagTest& operator=(CorrectAndFlagTest&&)      = delete;
  ~CorrectAndFlagTest() override                           = default;

  void run() override;

private:
  void testDivergedPredictorMarksTheCellTroubled();
  void testAdmissibleCandidateKeepsTheGalerkinScheme();
  void testInadmissibleCandidateReachesTheAdjacentFaces();
  void testAdaptiveSolverTakesTheWaveSpeedOfAnAcceptedCandidate();
  void testTheNeighbourhoodRangeDecidesTheMaximumPrinciple();
  void testAnAdmissibleCellWithATroubledHistoryStaysLimited();
  void testACellWithoutARegularPreviousStatusSkipsTheWaveSpeed();
  void testAFixedStepSolverTakesTheVerdictWithoutAWaveSpeed();
};
