// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PlanarAcousticSolver.h"

void tutorials::exahype2::acoustic::PlanarAcousticSolver::initialCondition(
  [[maybe_unused]] double* const NOALIAS                        Q, // Q[3+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
) {
  // Simple translation in positive diagonal direction
  const double val = cos(-std::numbers::pi * (x[0] + x[1]));

  Q[Shortcuts::v + 0] = val;
  Q[Shortcuts::v + 1] = val;

  constexpr double K0  = 4.0;
  constexpr double rho = 1.0;

  // These are defined by the eigenvector of the plane wave operator
  const double kr = std::sqrt(2 * K0 * rho);
  Q[Shortcuts::p] = kr * val;
}

void tutorials::exahype2::acoustic::PlanarAcousticSolver::boundaryConditions(
  [[maybe_unused]] const double* const NOALIAS                  Qinside,  // Qinside[3+0]
  [[maybe_unused]] double* const NOALIAS                        Qoutside, // Qoutside[3+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal
) {
  // Since we are using periodic boundary conditions, this need never be called.
  assertion(false);
}

double tutorials::exahype2::acoustic::PlanarAcousticSolver::maxEigenvalue(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[3+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal
) {
  constexpr double K0  = 4.0;
  constexpr double rho = 1.0;
  const double     c   = std::sqrt(4.0 / rho);
  return c;
}

void tutorials::exahype2::acoustic::PlanarAcousticSolver::flux(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[3+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const NOALIAS                        F // F[3]
) {
  constexpr double K0  = 4.0;
  constexpr double rho = 1.0;

  switch (normal) {
  case 0: // Flux in x-direction
    F[Shortcuts::p]     = K0 * Q[Shortcuts::v + normal];
    F[Shortcuts::v + 0] = Q[Shortcuts::p] / rho;
    F[Shortcuts::v + 1] = 0.0;
    break;
  case 1: // Flux in y-direction
    F[Shortcuts::p]     = K0 * Q[Shortcuts::v + normal];
    F[Shortcuts::v + 0] = 0.0;
    F[Shortcuts::v + 1] = Q[Shortcuts::p] / rho;
  }
}
