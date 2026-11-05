// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/fv/reconstruction/weno/NonlinearWeights.h"
#include "tarch/testing/UnitTest.h"

namespace {
  constexpr int    Stencils                = 3;
  constexpr double LinearWeights[Stencils] = {1.0, 1e5, 1.0};
} // namespace

UNIT_TEST("Jiang-Shu weights sum to one") {
  const double indicators[Stencils] = {0.3, 0.9, 2.0};
  double       weights[Stencils];
  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, LinearWeights, indicators, weights);

  VALIDATE_NUMERICAL_EQUALS(weights[0] + weights[1] + weights[2], 1.0);
  for (int stencil = 0; stencil < Stencils; stencil++) {
    VALIDATE(weights[stencil] > 0.0, stencil, weights[stencil]);
  }
  // The one-sided stencil with the smaller indicator outweighs the other one.
  VALIDATE(weights[0] > weights[2]);
}

UNIT_TEST("Jiang-Shu equal indicators reproduce linear weights") {
  const double indicators[Stencils] = {0.5, 0.5, 0.5};
  double       weights[Stencils];
  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, LinearWeights, indicators, weights);

  const double sum = LinearWeights[0] + LinearWeights[1] + LinearWeights[2];
  for (int stencil = 0; stencil < Stencils; stencil++) {
    VALIDATE_NUMERICAL_EQUALS(weights[stencil], LinearWeights[stencil] / sum, stencil);
  }
}

UNIT_TEST("Jiang-Shu rough stencil loses its weight") {
  const double indicators[Stencils] = {1e-6, 10.0, 1e-6};
  double       weights[Stencils];
  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, LinearWeights, indicators, weights);

  VALIDATE(weights[1] < 1e-9, weights[1]);
  VALIDATE_NUMERICAL_EQUALS(weights[0], weights[2]);
}

/** Over smooth data all indicators nearly vanish, and the huge central linear weight wins. */
UNIT_TEST("Jiang-Shu smooth data selects central stencil") {
  const double indicators[Stencils] = {2e-12, 1e-12, 2e-12};
  double       weights[Stencils];
  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, LinearWeights, indicators, weights);

  VALIDATE(weights[1] > 0.99, weights[1]);
}

/** Twice the indicator of another stencil costs a stencil the factor 2^Exponent of its weight. */
UNIT_TEST("Jiang-Shu exponent sets sensitivity") {
  const double linear[Stencils]     = {1.0, 1.0, 1.0};
  const double indicators[Stencils] = {0.5, 1.0, 0.5};
  double       weights[Stencils];

  exahype2::fv::reconstruction::weno::JiangShu<2>::weights(Stencils, linear, indicators, weights);
  VALIDATE_NUMERICAL_EQUALS(weights[1] / weights[0], 0.25);

  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, linear, indicators, weights);
  VALIDATE_NUMERICAL_EQUALS(weights[1] / weights[0], 1.0 / 256.0);
}

UNIT_TEST("Single precision") {
  const float linear[Stencils]     = {1.0f, 1e5f, 1.0f};
  const float indicators[Stencils] = {0.0f, 0.0f, 4.0f};
  float       weights[Stencils];
  exahype2::fv::reconstruction::weno::JiangShu<8>::weights(Stencils, linear, indicators, weights);

  VALIDATE(std::abs(weights[0] + weights[1] + weights[2] - 1.0f) < 1e-6f, weights[0] + weights[1] + weights[2]);
  VALIDATE(std::isfinite(weights[0]) and std::isfinite(weights[1]) and std::isfinite(weights[2]));
  VALIDATE(weights[1] > weights[0]);
  VALIDATE(weights[0] > weights[2]);
}
