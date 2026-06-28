// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// Worked ADER-DG implementation for the 3D linear elastic wave equations in
// velocity-stress form. An isotropic explosion at the centre of the cube radiates
// an outgoing spherical P-wave. The unknowns are addressed through the generated
// Shortcuts enum:
//   Shortcuts::v        velocity   (3 components: +0/+1/+2 = x/y/z)
//   Shortcuts::sigma    stress     (6 components, order xx, yy, zz, xy, xz, yz)
#include "ADERDGSolver.h"

#include <cmath>

namespace {
  constexpr double Rho    = 1.0;                        // density
  constexpr double Cp     = 2.0;                        // P-wave (compressional) speed
  constexpr double Cs     = 1.0;                        // S-wave (shear) speed
  constexpr double Mu     = Rho * Cs * Cs;              // Lamé parameter mu  = rho cs^2
  constexpr double Lambda = Rho * Cp * Cp - 2.0 * Mu;   // Lamé parameter lambda = rho cp^2 - 2 mu
  constexpr double iRho   = 1.0 / Rho;
} // namespace

void tutorials::exahype2::elastic::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  for (int i = 0; i < NumberOfUnknowns; i++) {
    Q[i] = 0.0;
  }
  // Isotropic explosion: a Gaussian pressure bump (equal diagonal stresses) at
  // the centre of the cube, at rest. It radiates an outgoing spherical P-wave.
  const double r2 = cellCentre[0] * cellCentre[0]
                  + cellCentre[1] * cellCentre[1]
                  + cellCentre[2] * cellCentre[2];
  constexpr double w = 0.1;
  const double bump  = std::exp(-r2 / (2.0 * w * w));
  Q[Shortcuts::sigma + 0] = bump;   // sigma_xx
  Q[Shortcuts::sigma + 1] = bump;   // sigma_yy
  Q[Shortcuts::sigma + 2] = bump;   // sigma_zz
}

void tutorials::exahype2::elastic::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                  Qinside,
  double* const NOALIAS                        Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const int                                    normal
) {
  // Non-periodic walls: a quiescent exterior, off which the waves reflect.
  for (int i = 0; i < NumberOfUnknowns; i++) {
    Qoutside[i] = 0.0;
  }
}

double tutorials::exahype2::elastic::ADERDGSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal
) {
  // The fastest wave is the P-wave, so the largest eigenvalue is c_p.
  return Cp;
}

void tutorials::exahype2::elastic::ADERDGSolver::flux(
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

  // The velocity-stress flux in the requested direction. Velocities are driven by
  // the divergence of the stress; stresses by the strain rate (Hooke's law).
  switch (normal) {
  case 0: // x
    F[Shortcuts::v + 0]     = -iRho * Q[Shortcuts::sigma + 0];
    F[Shortcuts::v + 1]     = -iRho * Q[Shortcuts::sigma + 3];
    F[Shortcuts::v + 2]     = -iRho * Q[Shortcuts::sigma + 4];
    F[Shortcuts::sigma + 0] = -(Lambda + 2.0 * Mu) * vx;
    F[Shortcuts::sigma + 1] = -Lambda * vx;
    F[Shortcuts::sigma + 2] = -Lambda * vx;
    F[Shortcuts::sigma + 3] = -Mu * vy;
    F[Shortcuts::sigma + 4] = -Mu * vz;
    break;
  case 1: // y
    F[Shortcuts::v + 0]     = -iRho * Q[Shortcuts::sigma + 3];
    F[Shortcuts::v + 1]     = -iRho * Q[Shortcuts::sigma + 1];
    F[Shortcuts::v + 2]     = -iRho * Q[Shortcuts::sigma + 5];
    F[Shortcuts::sigma + 0] = -Lambda * vy;
    F[Shortcuts::sigma + 1] = -(Lambda + 2.0 * Mu) * vy;
    F[Shortcuts::sigma + 2] = -Lambda * vy;
    F[Shortcuts::sigma + 3] = -Mu * vx;
    F[Shortcuts::sigma + 5] = -Mu * vz;
    break;
  case 2: // z
    F[Shortcuts::v + 0]     = -iRho * Q[Shortcuts::sigma + 4];
    F[Shortcuts::v + 1]     = -iRho * Q[Shortcuts::sigma + 5];
    F[Shortcuts::v + 2]     = -iRho * Q[Shortcuts::sigma + 2];
    F[Shortcuts::sigma + 0] = -Lambda * vz;
    F[Shortcuts::sigma + 1] = -Lambda * vz;
    F[Shortcuts::sigma + 2] = -(Lambda + 2.0 * Mu) * vz;
    F[Shortcuts::sigma + 4] = -Mu * vx;
    F[Shortcuts::sigma + 5] = -Mu * vy;
    break;
  }
}
