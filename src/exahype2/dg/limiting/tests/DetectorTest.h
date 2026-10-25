// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class DetectorTest;
} // namespace exahype2::dg::limiting::tests

class exahype2::dg::limiting::tests::DetectorTest final : public tarch::tests::TestCase {
public:
  DetectorTest();
  DetectorTest(const DetectorTest& other)            = delete;
  DetectorTest(DetectorTest&& other)                 = delete;
  DetectorTest& operator=(const DetectorTest& other) = delete;
  DetectorTest& operator=(DetectorTest&& other)      = delete;
  ~DetectorTest() override                           = default;

  void run() override;

private:
  void testTheExtremaMergesKeepNaNAndSignedZero();
  void testTheDiscreteMaximumPrinciple();
  void testTheExtremumQuantisation();
  void testTheObservableExtremaCoverEveryState();
  void testTheProjectedObservableExtremaCoverTheSamples();
  void testTheCellExtremaCoverTheNodesAndTheSamples();
  void testTheRelaxationTermReplacesTheSpecifiedParameter();
  void testACandidateInsideTheReferenceExtremaIsAccepted();
  void testACandidateOutsideTheReferenceExtremaIsRejected();
  void testTheRelaxationWidensTheAcceptedRange();
  void testTheAdmissibilityCombinesNodesAndSubcells();
  void testTheAdmissibilityCoversTheLobattoSamples();
  void testWidenExtremaMergesTheMinimaAndTheMaxima();
  void testOpenInitialStatusMarksAnOscillatingInterpolant();
  void testOpenStepCopiesTheSolutionAndOpensTheStatus();
  void testJudgeCandidateMarksAnInadmissibleCellTroubledAndWritesTheFaces();
  void testJudgeCandidateKeepsATroubledCellLimited();
  void testJudgeCandidateReportsTheWaveSpeedsOfAnAcceptedCell();
  void testJudgeCandidateFailsADivergedPredictor();
  void testJudgeCandidateRejectsACandidateOutsideTheReferenceExtrema();
  void testSpreadStatusLiftsByTheNeighbours();
  void testSpreadStatusesReadsThePreviousRing();
  void testClearFacesWritesRegularAndZerosTheSubcellHalo();
  void testAFixedRuleReportsNoWaveSpeeds();
  void testEveryCellOfADeviceRangeJudgesLikeTheHost();
  void testEveryTeamOfADeviceJudgesLikeTheHost();
  void testEveryTeamOfADeviceReducesTheExtremaLikeTheHost();
  void testTheTeamsOfALaunchOpenTheInitialStatusesLikeTheHost();
  void testTheTeamsOfALaunchCoverTheCells();
  void testTheCellListsStartWithTheLocalCells();
  void testTheStatusTransferCopiesTheParentAndRaisesToTheChildren();
};
