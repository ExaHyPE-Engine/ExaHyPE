// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class SubcellLimiterTest;
} // namespace exahype2::dg::limiting::tests

class exahype2::dg::limiting::tests::SubcellLimiterTest final : public tarch::tests::TestCase {
public:
  SubcellLimiterTest();
  SubcellLimiterTest(const SubcellLimiterTest&)            = delete;
  SubcellLimiterTest(SubcellLimiterTest&&)                 = delete;
  SubcellLimiterTest& operator=(const SubcellLimiterTest&) = delete;
  SubcellLimiterTest& operator=(SubcellLimiterTest&&)      = delete;
  ~SubcellLimiterTest() override                           = default;

  void run() override;

private:
  void testAnAcceptedFreeStreamStaysOnTheGalerkinSchemeBitwise();
  void testARejectedFreeStreamRecomputesEveryCellAndKeepsTheConstant();
  void testOneTroubledCellSpreadsTwoRingsOfNeighbours();
  void testReleaseFreesEveryPool();
};
