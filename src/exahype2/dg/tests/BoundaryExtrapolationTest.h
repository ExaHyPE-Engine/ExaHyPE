// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class BoundaryExtrapolationTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::BoundaryExtrapolationTest final : public tarch::tests::TestCase {
public:
  BoundaryExtrapolationTest();
  BoundaryExtrapolationTest(const BoundaryExtrapolationTest&)            = delete;
  BoundaryExtrapolationTest(BoundaryExtrapolationTest&&)                 = delete;
  BoundaryExtrapolationTest& operator=(const BoundaryExtrapolationTest&) = delete;
  BoundaryExtrapolationTest& operator=(BoundaryExtrapolationTest&&)      = delete;
  ~BoundaryExtrapolationTest() override                                  = default;

  void run() override;

private:
  void testFaceBuffersAreClearedBeforeTheProjection();
  void testAConstantStateReachesEveryFace();
  void testEachFaceNodeSeesOnlyItsOwnLine();
  void testTheTwoEndsUseTheirOwnCoefficients();
  void testFluxIsEvaluatedAtCellNodesBeforeExtrapolation();
  void testASystemWithoutFluxLeavesZeroesBehind();
  void testFluxOnAFaceComesFromTheFaceState();
  void testDiffusiveFluxOnAFaceReadsTheJumpAlongTheNormal();
  void testDiffusiveFluxIsExtrapolatedWithTheNodalGradients();
  void testAPaddedNodeStrideProjectsOnlyTheEntries();
  void testTheLowerAndUpperFaceUseTheirOwnCoefficients();
  void testTheFaceNodesRunWithTheLowestTangentialAxisFastest();
  void testAFieldOfFloatAccumulatesTheFacesInDouble();
};
