// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

namespace {
  constexpr double Rho        = 1.0;
  constexpr double Cp         = 2.0;                      // Compressional wave speed
  constexpr double Cs         = 1.0;                      // Shear wave speed
  constexpr double Mu         = Rho * Cs * Cs;            // Lamé parameter mu
  constexpr double Lambda     = Rho * Cp * Cp - 2.0 * Mu; // Lamé parameter lambda
  constexpr double InverseRho = 1.0 / Rho;
} // namespace

void
tutorials::exahype2::elastic::ADERDGSolver::initialCondition(
  State&                   Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const RuntimeParameters& parameters
) {
  const double     r2 = cellCentre[0] * cellCentre[0] + cellCentre[1] * cellCentre[1] + cellCentre[2] * cellCentre[2];
  constexpr double Width = 0.1;
  const double     bump  = std::exp(-r2 / (2.0 * Width * Width));
  Q                      = {};
  Q.sigma(0, 0)          = bump; // sigma_xx
  Q.sigma(1, 1)          = bump; // sigma_yy
  Q.sigma(2, 2)          = bump; // sigma_zz
}

void
tutorials::exahype2::elastic::ADERDGSolver::boundaryConditions(
  const State&             Qinside,
  State&                   Qoutside,
  const Coordinates&       faceCentre,
  const Coordinates&       cellSize,
  const double             time,
  const int                normal,
  const RuntimeParameters& parameters
) {
  Qoutside = {};
}

double
tutorials::exahype2::elastic::ADERDGSolver::maxEigenvalue(
  const State&             Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const double             time,
  const double             timeStepSize,
  const int                normal,
  const RuntimeParameters& parameters
) {
  return Cp;
}

void
tutorials::exahype2::elastic::ADERDGSolver::flux(
  const State&             Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const double             time,
  const double             timeStepSize,
  const int                normal,
  Flux&                    F,
  const RuntimeParameters& parameters
) {
  F = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    F.v(axis)           = -InverseRho * Q.sigma(normal, axis);
    F.sigma(axis, axis) = -Lambda * Q.v(normal);
    if (axis != normal) {
      F.sigma(normal, axis) = -Mu * Q.v(axis);
    }
  }
  F.sigma(normal, normal) = -(Lambda + 2.0 * Mu) * Q.v(normal);
}
