#include "../../selfgravitation/accretion-through-approximated-rhs/GravityModel.h"


void applications::exahype2::euler::sphericalaccretion::initialiseHomogeneousDensity(
  double* __restrict__ Q, double baselineDensity, double baselinePressure, double gamma
) {
  Q[0] = baselineDensity;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;
  Q[4] = 0.0;
}


void applications::exahype2::euler::sphericalaccretion::initialiseOverdensity_topHat(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  double                                       topHatRadius,
  double                                       additionalMass,
  double                                       baselineDensity,
  double                                       baselinePressure,
  double                                       gamma
) {
  double radius = tarch::la::norm2(x);

  double additionalDensity = (radius < topHatRadius)
                               ? additionalMass * 3.0 / 4.0 / tarch::la::PI / std::pow(topHatRadius, 3.0)
                               : 0.0;


  Q[0] = baselineDensity + additionalDensity;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;
  Q[4] = 0.5 * baselinePressure / (gamma - 1);
}


void applications::exahype2::euler::sphericalaccretion::initialiseOverdensity_Gaussian(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  double                                       topHatRadius,
  double                                       additionalMass,
  double                                       baselineDensity,
  double                                       baselinePressure,
  double                                       gamma
) {
  double standardDeviation = topHatRadius;
  double gaussianScaling   = 1.0;

  double radius = tarch::la::norm2(x);

  double exponent = -0.5 * (radius / standardDeviation) * (radius / standardDeviation);

  double gaussian = 1.0 / (standardDeviation * std::sqrt(2.0 * tarch::la::PI)) * std::exp(exponent);

  double additionalDensity = additionalMass * gaussian * gaussianScaling;

  Q[0] = baselineDensity + additionalDensity;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;
  Q[4] = 0.5 * baselinePressure / (gamma - 1);
}


void applications::exahype2::euler::sphericalaccretion::initialiseOverdensity_bumpFunction(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  double                                       topHatRadius,
  double                                       additionalMass,
  double                                       baselineDensity,
  double                                       baselinePressure,
  double                                       gamma
) {
  double radius = tarch::la::norm2(x);

  double topHatAdditionalDensity = additionalMass * 3.0 / 4.0 / tarch::la::PI / std::pow(topHatRadius, 3.0);

  double scalingOfTopHat = 1.2;
  double rNormalised     = radius / (scalingOfTopHat * topHatRadius);

  double scaling = tarch::la::smallerEquals(rNormalised, 1.0) ? std::exp(-1.0 / (1 - rNormalised * rNormalised)) : 0.0;

  double additionalDensity = topHatAdditionalDensity * scaling;

  Q[0] = baselineDensity + additionalDensity;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;
  Q[4] = 0.5 * baselinePressure / (gamma - 1);
}


void applications::exahype2::euler::sphericalaccretion::initialiseOverdensity_hyperbolicSecant(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  double                                       topHatRadius,
  double                                       additionalMass,
  double                                       baselineDensity,
  double                                       baselinePressure,
  double                                       gamma
) {
  double radius = tarch::la::norm2(x);

  double topHatAdditionalDensity = additionalMass * 3.0 / 4.0 / tarch::la::PI / std::pow(topHatRadius, 3.0);

  double scaledRadius      = radius / topHatRadius;
  double volumetricScaling = 1e-1;

  double additionalDensity = topHatAdditionalDensity * 2.0 / (std::exp(scaledRadius) + std::exp(-scaledRadius))
                             * volumetricScaling;

  assertion(additionalDensity >= 0.0);

  Q[0] = baselineDensity + additionalDensity;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;
  Q[4] = 0.5 * baselinePressure / (gamma - 1);
}


void applications::exahype2::euler::sphericalaccretion::addGravitationalSource_AlphaCDM(
  double* __restrict__ S,
  const tarch::la::Vector<Dimensions, double>& x,
  const double* __restrict__ Q,
  double mass,
  double aInitial,
  double t
) {
  double radius = tarch::la::norm2(x);

  if (tarch::la::greater(radius, 0.0)) {
    double a = 1e-3 * 0.0287 * std::pow((-t / 11.8 + 0.1694 * std::pow(aInitial, -0.5)), -2); // when code time ~
                                                                                              // 2*(a_i^(-0.5)-1), a~1

    double normalisedDistance = std::max(1.0, radius);

    double forceDensity = a * Q[0] * mass / std::pow(normalisedDistance, 3.0);

    S[1] += -forceDensity * x(0) / radius; // velocities
    S[2] += -forceDensity * x(1) / radius; // velocities
    S[3] += -forceDensity * x(2) / radius; // velocities
    S[4] += -forceDensity * (Q[1] * x(0) + Q[2] * x(1) + Q[3] * x(2)) / radius / Q[0];
  }
}
