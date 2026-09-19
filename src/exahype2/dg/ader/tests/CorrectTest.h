// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class CorrectTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the cell and face bodies of the ADER-DG correction.
 */
class exahype2::dg::ader::tests::CorrectTest final : public tarch::tests::TestCase {
public:
  CorrectTest();
  CorrectTest(const CorrectTest&)            = delete;
  CorrectTest(CorrectTest&&)                 = delete;
  CorrectTest& operator=(const CorrectTest&) = delete;
  CorrectTest& operator=(CorrectTest&&)      = delete;
  ~CorrectTest() override                    = default;

  void run() override;

private:
  void testTheRiemannSolverWritesBothFaceHalves();
  void testAHangingFaceReadsTheCoarseStampAndKeepsItsFlags();
  void testTheBoundaryFaceOfARefinedCellKeepsItsFluxes();
  void testHangingFaceStampsRiseToTheCoarseFace();
  void testTheFaceIntegralsReachTheCellState();
  void testAFaceAboveAHangingNeighbourIsReadFromTheOtherHalf();
  void testTheCorrectionCommitsTheTimeStep();
  void testAnUnrefinedBoundaryFaceTakesTheBoundaryState();
  void testAPeriodicBoundaryFaceIsSolvedAsAnInteriorFace();
  void testTheSolversRiemannSolverReceivesTheFace();
  void testTheLinearRiemannSolverWritesOppositeDissipations();
};
