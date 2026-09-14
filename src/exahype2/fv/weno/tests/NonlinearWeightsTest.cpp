// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonlinearWeightsTest.h"

#include "exahype2/fv/weno/NonlinearWeights.h"

namespace {
  constexpr int    Stencils               = exahype2::fv::weno::NumberOfCandidateStencils;
  constexpr double IdealWeights[Stencils] = {0.1, 0.6, 0.3};
} // namespace

exahype2::fv::weno::tests::NonlinearWeightsTest::NonlinearWeightsTest() :
  tarch::tests::TestCase("exahype2::fv::weno::tests::NonlinearWeightsTest") {}

void exahype2::fv::weno::tests::NonlinearWeightsTest::run() {
  testMethod(testJiangShuWeightsSumToOne);
  testMethod(testJiangShuEqualIndicatorsReproduceIdealWeights);
  testMethod(testJiangShuRoughStencilLosesItsWeight);
  testMethod(testSinglePrecision);
}

void exahype2::fv::weno::tests::NonlinearWeightsTest::testJiangShuWeightsSumToOne() {
  const double smoothness[Stencils] = {0.3, 0.01, 2.0};
  double       weights[Stencils];
  exahype2::fv::weno::JiangShu::weights(IdealWeights, smoothness, weights);

  validateNumericalEquals(weights[0] + weights[1] + weights[2], 1.0);
  for (int stencil = 0; stencil < Stencils; stencil++) {
    validateWithParams2(weights[stencil] > 0.0, stencil, weights[stencil]);
  }
  // The smoothest stencil dominates and the roughest one trails.
  validate(weights[1] > weights[0]);
  validate(weights[0] > weights[2]);
}

void exahype2::fv::weno::tests::NonlinearWeightsTest::testJiangShuEqualIndicatorsReproduceIdealWeights() {
  const double smoothness[Stencils] = {0.5, 0.5, 0.5};
  double       weights[Stencils];
  exahype2::fv::weno::JiangShu::weights(IdealWeights, smoothness, weights);

  for (int stencil = 0; stencil < Stencils; stencil++) {
    validateNumericalEqualsWithParams1(weights[stencil], IdealWeights[stencil], stencil);
  }
}

void exahype2::fv::weno::tests::NonlinearWeightsTest::testJiangShuRoughStencilLosesItsWeight() {
  const double smoothness[Stencils] = {1e-12, 1e-12, 1.0};
  double       weights[Stencils];
  exahype2::fv::weno::JiangShu::weights(IdealWeights, smoothness, weights);

  validateWithParams1(weights[2] < 1e-9, weights[2]);
  // The two smooth stencils keep the ratio of their ideal weights.
  validateNumericalEquals(weights[0] / weights[1], IdealWeights[0] / IdealWeights[1]);
}

void exahype2::fv::weno::tests::NonlinearWeightsTest::testSinglePrecision() {
  const float ideal[Stencils]      = {0.1f, 0.6f, 0.3f};
  const float smoothness[Stencils] = {0.1f, 0.5f, 4.0f};
  float       weights[Stencils];
  exahype2::fv::weno::JiangShu::weights(ideal, smoothness, weights);

  validateWithParams1(
    std::abs(weights[0] + weights[1] + weights[2] - 1.0f) < 1e-6f,
    weights[0] + weights[1] + weights[2]
  );
  validate(weights[0] > weights[1]);
  validate(weights[1] > weights[2]);
}
