// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::rk::tests {
  class InterpolateRiemannSolutionTest;
} // namespace exahype2::dg::rk::tests

class exahype2::dg::rk::tests::InterpolateRiemannSolutionTest final : public tarch::tests::TestCase {
public:
  InterpolateRiemannSolutionTest();
  InterpolateRiemannSolutionTest(const InterpolateRiemannSolutionTest&)            = delete;
  InterpolateRiemannSolutionTest(InterpolateRiemannSolutionTest&&)                 = delete;
  InterpolateRiemannSolutionTest& operator=(const InterpolateRiemannSolutionTest&) = delete;
  InterpolateRiemannSolutionTest& operator=(InterpolateRiemannSolutionTest&&)      = delete;
  ~InterpolateRiemannSolutionTest() override                                       = default;

  void run() override;

private:
  void testCellWithoutHangingFaceKeepsEveryFace();
  void testHangingFaceTakesTheCoarseTraceAndTheCellTrace();
  void testHangingFaceSolvesTheRiemannProblemAtTheStageTimeStamp();
  void testUpperHangingFaceTakesTheCoarseTraceOfItsChildPosition();
  void testPeriodicHangingFaceCallsTheInteriorRiemannSolver();
};
