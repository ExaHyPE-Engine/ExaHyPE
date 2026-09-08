// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestUtils.h"

void exahype2::dg::tests::eulerFlux(
  [[maybe_unused]] const double* NOALIAS                        Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* NOALIAS                              F
) {
  constexpr double Gamma          = 1.4;
  const double     inverseDensity = 1. / Q[0];
  const double     pressure = (Gamma - 1) * (Q[4] - 0.5 * inverseDensity * (Q[1] * Q[1] + Q[2] * Q[2] + Q[3] * Q[3]));

  const double velocity = inverseDensity * Q[normal + 1];
  F[0]                  = velocity * Q[0];
  F[1]                  = velocity * Q[1];
  F[2]                  = velocity * Q[2];
  F[3]                  = velocity * Q[3];
  F[normal + 1] += pressure;
  F[4] = velocity * (Q[4] + pressure);
}

void exahype2::dg::tests::eulerSource(
  [[maybe_unused]] const double* NOALIAS                        Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* NOALIAS                              S
) {
  S[0] = 0.0;
  S[1] = 0.0;
  S[2] = 0.0;
  S[3] = 0.0;
  S[4] = 0.0;
}

double exahype2::dg::tests::eulerEigenvalue(
  [[maybe_unused]] const double* NOALIAS                        Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  assertion(Q[0] > 0.0, Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal);
  const double inverseDensity = 1.0 / Q[0];

  constexpr double Gamma    = 1.4;
  const double     pressure = (Gamma - 1) * (Q[4] - 0.5 * inverseDensity * (Q[1] * Q[1] + Q[2] * Q[2] + Q[3] * Q[3]));

  assertion(pressure >= 0.0, Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal);
  const double soundSpeed = std::sqrt(Gamma * pressure * inverseDensity);

  double maximumEigenvalue = 1.0;
  switch (normal) {
  case 0: // x
    maximumEigenvalue =
      std::max(std::abs(Q[1] * inverseDensity - soundSpeed), std::abs(Q[1] * inverseDensity + soundSpeed));
    break;
  case 1: // y
    maximumEigenvalue =
      std::max(std::abs(Q[2] * inverseDensity - soundSpeed), std::abs(Q[2] * inverseDensity + soundSpeed));
    break;
  case 2: // z
    maximumEigenvalue =
      std::max(std::abs(Q[3] * inverseDensity - soundSpeed), std::abs(Q[3] * inverseDensity + soundSpeed));
  }
  return maximumEigenvalue;
}

void exahype2::dg::tests::eulerBoundaryConditions(
  [[maybe_unused]] const double* NOALIAS                        Qinside,
  [[maybe_unused]] double* NOALIAS                              Qoutside,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];
}

void exahype2::dg::tests::eulerInitial([[maybe_unused]] double* NOALIAS Q, [[maybe_unused]] int node) {
  Q[0] = 1.00000000000000;
  Q[1] = 0.100000000000000;
  Q[2] = 0.200000000000000;
  Q[3] = 0.300000000000000;
  Q[4] = 2.57000000000000;
}

void exahype2::dg::tests::elasticInitial(double* NOALIAS Q) {
  for (int stateIndex = 0; stateIndex < 9; stateIndex++) {
    Q[stateIndex] = 1.0; // Six stresses followed by three velocities.
  }
  Q[9]  = 4.0; // rho
  Q[10] = 2.0; // cp
  Q[11] = 2.6; // cs
}

void exahype2::dg::tests::elasticFlux(
  [[maybe_unused]] const double* NOALIAS                        Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* NOALIAS                              F
) {
  for (int stressIndex = 0; stressIndex < 6; stressIndex++) {
    F[stressIndex] = 0;
  }

  switch (normal) {
  case 0:        // x
    F[6] = Q[0]; // xx
    F[7] = Q[3]; // xy
    F[8] = Q[4]; // xz
    break;
  case 1:        // y
    F[6] = Q[3]; // xy
    F[7] = Q[1]; // yy
    F[8] = Q[5]; // yz
    break;
  case 2:        // z
    F[6] = Q[4]; // xz
    F[7] = Q[5]; // yz
    F[8] = Q[2]; // zz
    break;
  }
}

void exahype2::dg::tests::elasticSource(
  [[maybe_unused]] const double* NOALIAS                        Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* NOALIAS                              S
) {
  S[0] = 1.0;
  S[1] = 1.0;
  S[2] = 1.0;
  S[3] = 1.0;
  S[4] = 1.0;
  S[5] = 1.0;
  S[6] = 1.0;
  S[7] = 1.0;
  S[8] = 1.0;
}

double exahype2::dg::tests::elasticEigenvalue(
  [[maybe_unused]] const double* NOALIAS                        Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  return std::max(std::abs(Q[10]), std::abs(Q[11]));
}

void exahype2::dg::tests::elasticBoundaryConditions(
  [[maybe_unused]] const double* NOALIAS                        Qinside,
  [[maybe_unused]] double* NOALIAS                              Qoutside,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  Qoutside[0]  = Qinside[0];
  Qoutside[1]  = Qinside[1];
  Qoutside[2]  = Qinside[2];
  Qoutside[3]  = Qinside[3];
  Qoutside[4]  = Qinside[4];
  Qoutside[5]  = Qinside[5];
  Qoutside[6]  = Qinside[6];
  Qoutside[7]  = Qinside[7];
  Qoutside[8]  = Qinside[8];
  Qoutside[9]  = Qinside[9];
  Qoutside[10] = Qinside[10];
  Qoutside[11] = Qinside[11];
}

void exahype2::dg::tests::elasticNonConservativeProduct(
  [[maybe_unused]] const double* NOALIAS                        Q,
  [[maybe_unused]] const double* NOALIAS                        deltaQ,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* NOALIAS                              BgradQ // BgradQ[9]
) {
  const double shearModulus       = Q[9] * Q[11] * Q[11];
  const double firstLameParameter = Q[9] * Q[10] * Q[10] - 2 * shearModulus;

  switch (normal) {
  case 0:                                                            // x
    BgradQ[0] = (firstLameParameter + 2 * shearModulus) * deltaQ[6]; // xx
    BgradQ[1] = firstLameParameter * deltaQ[6];                      // yy
    BgradQ[2] = firstLameParameter * deltaQ[6];                      // zz
    BgradQ[3] = shearModulus * deltaQ[7];                            // xy
    BgradQ[4] = shearModulus * deltaQ[8];                            // xz
    BgradQ[5] = 0;                                                   // yz
    break;
  case 1:                                                            // y
    BgradQ[0] = firstLameParameter * deltaQ[7];                      // xx
    BgradQ[1] = (firstLameParameter + 2 * shearModulus) * deltaQ[7]; // yy
    BgradQ[2] = firstLameParameter * deltaQ[7];                      // zz
    BgradQ[3] = shearModulus * deltaQ[6];                            // xy
    BgradQ[4] = 0;                                                   // xz
    BgradQ[5] = shearModulus * deltaQ[8];                            // yz
    break;
  case 2:
    BgradQ[0] = firstLameParameter * deltaQ[8];                      // xx
    BgradQ[1] = firstLameParameter * deltaQ[8];                      // yy
    BgradQ[2] = (firstLameParameter + 2 * shearModulus) * deltaQ[8]; // zz
    BgradQ[3] = 0;                                                   // xy
    BgradQ[4] = shearModulus * deltaQ[6];                            // xz
    BgradQ[5] = shearModulus * deltaQ[7];                            // yz
    break;
  }

  BgradQ[6] = 0;
  BgradQ[7] = 0;
  BgradQ[8] = 0;
}

void exahype2::dg::tests::testBoundaryConditions(
  [[maybe_unused]] const double* NOALIAS                        Qinside,
  [[maybe_unused]] double* NOALIAS                              Qoutside,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  Qoutside[0] = Qinside[0];
  Qoutside[1] = x[0];
  Qoutside[2] = t;
  Qoutside[3] = normal;
}

double exahype2::dg::tests::testEigenvalue(
  [[maybe_unused]] const double* NOALIAS                        Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  return Q[0] * normal;
}

void exahype2::dg::tests::secondTestBoundaryConditions(
  [[maybe_unused]] const double* NOALIAS                        Qinside,
  [[maybe_unused]] double* NOALIAS                              Qoutside,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  Qoutside[0] = 1.0;
  Qoutside[1] = 2.0;
  Qoutside[2] = 3.0;
  Qoutside[3] = 4.0;
}
