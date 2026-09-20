// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class RecomputeSubcellsTest;
} // namespace exahype2::dg::limiting::tests

/**
 * Checks the finite-volume step of a limited cell and the halo it rebuilds at a level jump.
 */
class exahype2::dg::limiting::tests::RecomputeSubcellsTest final : public tarch::tests::TestCase {
public:
  RecomputeSubcellsTest();
  RecomputeSubcellsTest(const RecomputeSubcellsTest&)            = delete;
  RecomputeSubcellsTest(RecomputeSubcellsTest&&)                 = delete;
  RecomputeSubcellsTest& operator=(const RecomputeSubcellsTest&) = delete;
  RecomputeSubcellsTest& operator=(RecomputeSubcellsTest&&)      = delete;
  ~RecomputeSubcellsTest() override                              = default;

  void run() override;

private:
  void testCellOnTheGalerkinSchemeKeepsItsSolution();
  void testLimitedCellAdvancesItsSubgridAndProjectsItBack();
  void testAFixedStepSolverReportsNoWaveSpeed();
  void testAnInadmissibleSubgridRunsTheFallback();
  void testATransitionFaceOfTheCellTakesItsRebuiltHalo();
  void testHangingFaceHaloComesFromTheOwnFaceTouchingLayer();
  void testCoarseFaceHaloComesFromTheNeighbourTrace();
  void testTheUpperCoarseFaceHaloComesFromTheUpperNeighbourHalf();
};
