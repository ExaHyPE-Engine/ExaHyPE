// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class InitialiseSubcellsTest;
} // namespace exahype2::dg::limiting::tests

/**
 * Checks the sampling of the initial condition on the subgrid and the startup limiter verdict.
 */
class exahype2::dg::limiting::tests::InitialiseSubcellsTest final : public tarch::tests::TestCase {
public:
  InitialiseSubcellsTest();
  InitialiseSubcellsTest(const InitialiseSubcellsTest&)            = delete;
  InitialiseSubcellsTest(InitialiseSubcellsTest&&)                 = delete;
  InitialiseSubcellsTest& operator=(const InitialiseSubcellsTest&) = delete;
  InitialiseSubcellsTest& operator=(InitialiseSubcellsTest&&)      = delete;
  ~InitialiseSubcellsTest() override                               = default;

  void run() override;

private:
  void testSubcellsHoldTheInitialConditionAtTheirCentres();
  void testInterpolantWithinTheSampledRangeLeavesTheCellUntroubled();
  void testInterpolantOutsideTheSampledRangeMarksTheCellTroubled();
};
