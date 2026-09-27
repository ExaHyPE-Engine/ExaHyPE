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
  SolverTest(const SolverTest&)            = delete;
  SolverTest(SolverTest&&)                 = delete;
  SolverTest& operator=(const SolverTest&) = delete;
  SolverTest& operator=(SolverTest&&)      = delete;
  ~SolverTest() override                   = default;

  void run() override;

private:
  void testInitialisationWritesTheInitialConditionIntoEveryNode();
  void testATimeStepWithoutFluxKeepsTheSolutionAndAdvancesTheTimeStamp();
  void testARefinementAndTheEraseBackKeepTheSolution();
  void testRefinementReducesTheNextAdaptiveTimeStep();
  void testTwoCoupledSolversExchangeTheirTracesAndAdvanceTogether();
  void testTheCellAndFaceGroupsSplitAtTheRankBoundary();
  void testTheProjectionWritesTheTracesOfTheSolutionIntoBothHalvesOfEveryInteriorFace();
  void testTheProjectionOntoAPlaneWritesTheTracesOfTheSolutionIntoItsFaces();
  void testReleaseFreesEveryPool();
};
