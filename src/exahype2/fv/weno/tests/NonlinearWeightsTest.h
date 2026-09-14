// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::weno::tests {
  class NonlinearWeightsTest;
} // namespace exahype2::fv::weno::tests

class exahype2::fv::weno::tests::NonlinearWeightsTest final : public tarch::tests::TestCase {
public:
  NonlinearWeightsTest();

  void run() override;

private:
  void testJiangShuWeightsSumToOne();
  void testJiangShuEqualIndicatorsReproduceIdealWeights();
  void testJiangShuRoughStencilLosesItsWeight();
  void testSinglePrecision();
};
