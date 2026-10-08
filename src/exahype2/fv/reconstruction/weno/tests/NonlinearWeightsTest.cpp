// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonlinearWeightsTest.h"

#include "exahype2/fv/reconstruction/weno/NonlinearWeights.h"

namespace {
  constexpr int    Stencils                = 3;
  constexpr double LinearWeights[Stencils] = {1.0, 1e5, 1.0};
} // namespace

exahype2::fv::reconstruction::weno::tests::NonlinearWeightsTest::NonlinearWeightsTest() :
  tarch::tests::TestCase("exahype2::fv::reconstruction::weno::tests::NonlinearWeightsTest") {}

void exahype2::fv::reconstruction::weno::tests::NonlinearWeightsTest::run() {
  testMethod(testJiangShuWeightsSumToOne);
  testMethod(testJiangShuEqualIndicatorsReproduceLinearWeights);
  testMethod(testJiangShuRoughStencilLosesItsWeight);
  testMethod(testJiangShuSmoothDataSelectsCentralStencil);
  testMethod(testJiangShuExponentSetsSensitivity);
  testMethod(testSinglePrecision);
}

void exahype2::fv::reconstruction::weno::tests::NonlinearWeightsTest::testJiangShuWeightsSumToOne() {
  const double indicators[Stencils] = {0.3, 0.9, 2.0};
  double       weights[Stencils];
  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, LinearWeights, indicators, weights);

  validateNumericalEquals(weights[0] + weights[1] + weights[2], 1.0);
  for (int stencil = 0; stencil < Stencils; stencil++) {
    validateWithParams2(weights[stencil] > 0.0, stencil, weights[stencil]);
  }
  // The one-sided stencil with the smaller indicator outweighs the other one.
  validate(weights[0] > weights[2]);
}

void exahype2::fv::reconstruction::weno::tests::NonlinearWeightsTest::testJiangShuEqualIndicatorsReproduceLinearWeights() {
  const double indicators[Stencils] = {0.5, 0.5, 0.5};
  double       weights[Stencils];
  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, LinearWeights, indicators, weights);

  const double sum = LinearWeights[0] + LinearWeights[1] + LinearWeights[2];
  for (int stencil = 0; stencil < Stencils; stencil++) {
    validateNumericalEqualsWithParams1(weights[stencil], LinearWeights[stencil] / sum, stencil);
  }
}

void exahype2::fv::reconstruction::weno::tests::NonlinearWeightsTest::testJiangShuRoughStencilLosesItsWeight() {
  const double indicators[Stencils] = {1e-6, 10.0, 1e-6};
  double       weights[Stencils];
  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, LinearWeights, indicators, weights);

  validateWithParams1(weights[1] < 1e-9, weights[1]);
  validateNumericalEquals(weights[0], weights[2]);
}

/** Over smooth data all indicators nearly vanish, and the huge central linear weight wins. */
void exahype2::fv::reconstruction::weno::tests::NonlinearWeightsTest::testJiangShuSmoothDataSelectsCentralStencil() {
  const double indicators[Stencils] = {2e-12, 1e-12, 2e-12};
  double       weights[Stencils];
  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, LinearWeights, indicators, weights);

  validateWithParams1(weights[1] > 0.99, weights[1]);
}

/** Twice the indicator of another stencil costs a stencil the factor 2^Exponent of its weight. */
void exahype2::fv::reconstruction::weno::tests::NonlinearWeightsTest::testJiangShuExponentSetsSensitivity() {
  const double linear[Stencils]     = {1.0, 1.0, 1.0};
  const double indicators[Stencils] = {0.5, 1.0, 0.5};
  double       weights[Stencils];

  exahype2::fv::reconstruction::weno::JiangShu<2>::weights(Stencils, linear, indicators, weights);
  validateNumericalEquals(weights[1] / weights[0], 0.25);

  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, linear, indicators, weights);
  validateNumericalEquals(weights[1] / weights[0], 1.0 / 256.0);
}

void exahype2::fv::reconstruction::weno::tests::NonlinearWeightsTest::testSinglePrecision() {
  const float linear[Stencils]     = {1.0f, 1e5f, 1.0f};
  const float indicators[Stencils] = {0.0f, 0.0f, 4.0f};
  float       weights[Stencils];
  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, linear, indicators, weights);

  validateWithParams1(
    std::abs(weights[0] + weights[1] + weights[2] - 1.0f) < 1e-6f,
    weights[0] + weights[1] + weights[2]
  );
  validate(std::isfinite(weights[0]) and std::isfinite(weights[1]) and std::isfinite(weights[2]));
  validate(weights[1] > weights[0]);
  validate(weights[0] > weights[2]);
}
