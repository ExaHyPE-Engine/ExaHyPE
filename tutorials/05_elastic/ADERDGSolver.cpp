// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

namespace {
  constexpr double Rho        = 1.0;                      // density
  constexpr double Cp         = 2.0;                      // P-wave (compressional) speed
  constexpr double Cs         = 1.0;                      // S-wave (shear) speed
  constexpr double Mu         = Rho * Cs * Cs;            // Lamé parameter mu
  constexpr double Lambda     = Rho * Cp * Cp - 2.0 * Mu; // Lamé parameter lambda
  constexpr double InverseRho = 1.0 / Rho;
} // namespace

void
tutorials::exahype2::elastic::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  for (int i = 0; i < NumberOfUnknowns; i++) {
    Q[i] = 0.0;
  }
  const double     r2 = cellCentre[0] * cellCentre[0] + cellCentre[1] * cellCentre[1] + cellCentre[2] * cellCentre[2];
  constexpr double Width  = 0.1;
  const double     bump   = std::exp(-r2 / (2.0 * Width * Width));
  Q[Shortcuts::sigma + 0] = bump; // sigma_xx
  Q[Shortcuts::sigma + 1] = bump; // sigma_yy
  Q[Shortcuts::sigma + 2] = bump; // sigma_zz
}

void
tutorials::exahype2::elastic::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                  Qinside,
  double* const NOALIAS                        Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const int                                    normal
) {
  for (int i = 0; i < NumberOfUnknowns; i++) {
    Qoutside[i] = 0.0;
  }
}

double
tutorials::exahype2::elastic::ADERDGSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal
) {
  return Cp;
}

void
tutorials::exahype2::elastic::ADERDGSolver::flux(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  double* const NOALIAS                        F
) {
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
  const double vx = Q[Shortcuts::v + 0];
  const double vy = Q[Shortcuts::v + 1];
  const double vz = Q[Shortcuts::v + 2];

  switch (normal) {
  case 0: // x
    F[Shortcuts::v + 0]     = -InverseRho * Q[Shortcuts::sigma + 0];
    F[Shortcuts::v + 1]     = -InverseRho * Q[Shortcuts::sigma + 3];
    F[Shortcuts::v + 2]     = -InverseRho * Q[Shortcuts::sigma + 4];
    F[Shortcuts::sigma + 0] = -(Lambda + 2.0 * Mu) * vx;
    F[Shortcuts::sigma + 1] = -Lambda * vx;
    F[Shortcuts::sigma + 2] = -Lambda * vx;
    F[Shortcuts::sigma + 3] = -Mu * vy;
    F[Shortcuts::sigma + 4] = -Mu * vz;
    break;
  case 1: // y
    F[Shortcuts::v + 0]     = -InverseRho * Q[Shortcuts::sigma + 3];
    F[Shortcuts::v + 1]     = -InverseRho * Q[Shortcuts::sigma + 1];
    F[Shortcuts::v + 2]     = -InverseRho * Q[Shortcuts::sigma + 5];
    F[Shortcuts::sigma + 0] = -Lambda * vy;
    F[Shortcuts::sigma + 1] = -(Lambda + 2.0 * Mu) * vy;
    F[Shortcuts::sigma + 2] = -Lambda * vy;
    F[Shortcuts::sigma + 3] = -Mu * vx;
    F[Shortcuts::sigma + 5] = -Mu * vz;
    break;
  case 2: // z
    F[Shortcuts::v + 0]     = -InverseRho * Q[Shortcuts::sigma + 4];
    F[Shortcuts::v + 1]     = -InverseRho * Q[Shortcuts::sigma + 5];
    F[Shortcuts::v + 2]     = -InverseRho * Q[Shortcuts::sigma + 2];
    F[Shortcuts::sigma + 0] = -Lambda * vz;
    F[Shortcuts::sigma + 1] = -Lambda * vz;
    F[Shortcuts::sigma + 2] = -(Lambda + 2.0 * Mu) * vz;
    F[Shortcuts::sigma + 4] = -Mu * vx;
    F[Shortcuts::sigma + 5] = -Mu * vy;
    break;
  }
}
