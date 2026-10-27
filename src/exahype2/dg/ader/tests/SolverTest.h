// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class SolverTest;
} // namespace exahype2::dg::ader::tests

class exahype2::dg::ader::tests::SolverTest final : public tarch::tests::TestCase {
public:
  SolverTest();
  SolverTest(const SolverTest& other)            = delete;
  SolverTest(SolverTest&& other)                 = delete;
  SolverTest& operator=(const SolverTest& other) = delete;
  SolverTest& operator=(SolverTest&& other)      = delete;
  ~SolverTest() override                         = default;

  void run() override;

private:
  void testInitialisationWritesTheInitialConditionIntoEveryNode();
  void testATimeStepWithoutFluxKeepsTheSolutionAndAdvancesTheTimeStamp();
  void testThePostprocessingRunsOnceForEveryCellOfATimeStep();
  void testARefinementAndTheEraseBackKeepTheSolution();
  void testALinearTimeStepReadsNoFaceFluxAcrossALevelJump();
  void testRefinementReducesTheNextAdaptiveTimeStep();
  void testTwoCoupledSolversExchangeTheirTracesAndAdvanceTogether();
  void testACoupledSolverInterpolatesAndRestrictsOnlyTheLevelJumpsOfItsCells();
  void testTheCellAndFaceGroupsSplitAtTheRankBoundary();
  void testTheProjectionWritesTheTracesOfTheSolutionIntoBothHalvesOfEveryInteriorFace();
  void testTheProjectionOntoAPlaneWritesTheTracesOfTheSolutionIntoItsFaces();
  void testTheProjectionOntoAPlaneWritesBothHalvesOfTheCoarseFacesOfTheLevelJumps();
  void testReleaseFreesEveryPool();
};
