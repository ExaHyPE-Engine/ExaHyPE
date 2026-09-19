// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class PredictSubcellsTest;
} // namespace exahype2::dg::limiting::tests

/**
 * Checks how a cell opens its limiter step and seeds the extrema of its adjacent vertices.
 */
class exahype2::dg::limiting::tests::PredictSubcellsTest final : public tarch::tests::TestCase {
public:
  PredictSubcellsTest();
  PredictSubcellsTest(const PredictSubcellsTest&)            = delete;
  PredictSubcellsTest(PredictSubcellsTest&&)                 = delete;
  PredictSubcellsTest& operator=(const PredictSubcellsTest&) = delete;
  PredictSubcellsTest& operator=(PredictSubcellsTest&&)      = delete;
  ~PredictSubcellsTest() override                            = default;

  void run() override;

private:
  void testTroubledHistoryStartsTheStepOnTheSubcellScheme();
  void testUntroubledCellStartsTheStepOnTheGalerkinScheme();
  void testAcceptedExtremaReachTheAdjacentVertices();
};
