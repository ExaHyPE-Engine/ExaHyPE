// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// Worked ADER-DG implementation for the compressible Euler equations, set up as
// a 2D Riemann (shock) problem. The conserved variables are addressed through
// the generated Shortcuts enum:
//   Shortcuts::rho      density        (1 component)
//   Shortcuts::j        momentum       (2 components: +0 is x, +1 is y)
//   Shortcuts::E        total energy   (1 component)
#include "ADERDGSolver.h"

#include <cmath>

namespace {
  constexpr double Gamma         = 1.4;
  constexpr double ShockPosition = 0.111;
  constexpr double pZero         = 1.0e-4;   // pressure floor for the vacuum guard
} // namespace

void tutorials::exahype2::limiting::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  // Low-density, low-pressure gas in the bottom-left quadrant; dense, high-pressure
  // gas everywhere else. The discontinuity drives shocks and a contact.
  const bool   low = cellCentre[0] < -ShockPosition && cellCentre[1] < -ShockPosition;
  const double p   = low ? 0.1 : 1.0;

  Q[Shortcuts::rho]   = low ? 0.125 : 1.0;
  Q[Shortcuts::j + 0] = 0.0;
  Q[Shortcuts::j + 1] = 0.0;
  Q[Shortcuts::E]     = p / (Gamma - 1.0);
}

void tutorials::exahype2::limiting::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                  Qinside,
  double* const NOALIAS                        Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const int                                    normal
) {
  // Hold the initial outer state at the boundary (the shocks never reach it).
  const bool   low = faceCentre[0] < -ShockPosition && faceCentre[1] < -ShockPosition;
  const double p   = low ? 0.1 : 1.0;

  Qoutside[Shortcuts::rho]   = low ? 0.125 : 1.0;
  Qoutside[Shortcuts::j + 0] = 0.0;
  Qoutside[Shortcuts::j + 1] = 0.0;
  Qoutside[Shortcuts::E]     = p / (Gamma - 1.0);
}

void tutorials::exahype2::limiting::ADERDGSolver::flux(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  double* const NOALIAS                        F
) {
  const double irho = Q[Shortcuts::rho] > 0.0 ? 1.0 / Q[Shortcuts::rho] : 0.0;
  const double p    = (Gamma - 1.0)
    * (Q[Shortcuts::E] - 0.5 * irho * (Q[Shortcuts::j + 0] * Q[Shortcuts::j + 0] + Q[Shortcuts::j + 1] * Q[Shortcuts::j + 1]));

  if (p > pZero) {
    const double jn   = Q[Shortcuts::j + normal];
    F[Shortcuts::rho]   = jn;
    F[Shortcuts::j + 0] = jn * Q[Shortcuts::j + 0] * irho;
    F[Shortcuts::j + 1] = jn * Q[Shortcuts::j + 1] * irho;
    F[Shortcuts::E]     = jn * irho * (Q[Shortcuts::E] + p);
    F[Shortcuts::j + normal] += p;
  } else {
    // Non-physical state: emit no flux and let the limiter recompute the cell.
    F[Shortcuts::rho]   = 0.0;
    F[Shortcuts::j + 0] = 0.0;
    F[Shortcuts::j + 1] = 0.0;
    F[Shortcuts::E]     = 0.0;
  }
}

double tutorials::exahype2::limiting::ADERDGSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal
) {
  const double irho = Q[Shortcuts::rho] > 0.0 ? 1.0 / Q[Shortcuts::rho] : 0.0;
  const double p    = (Gamma - 1.0)
    * (Q[Shortcuts::E] - 0.5 * irho * (Q[Shortcuts::j + 0] * Q[Shortcuts::j + 0] + Q[Shortcuts::j + 1] * Q[Shortcuts::j + 1]));

  if (p > pZero) {
    const double c  = std::sqrt(Gamma * p * irho);
    const double un = Q[Shortcuts::j + normal] * irho;
    return std::fmax(std::fabs(un - c), std::fabs(un + c));
  }
  return 0.0;   // troubled cell: the limiter recomputes it with Finite Volumes
}
