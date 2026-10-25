// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// The functions implement the compressible Euler equations for a 2D Riemann problem. The subcell
// scheme of a troubled cell evaluates the same functions.
//
// Q stores the density at Shortcuts::rho, the momentum at Shortcuts::j + 0 and Shortcuts::j + 1,
// and the total energy at Shortcuts::E.
#include "ADERDGSolver.h"

namespace {
  constexpr double Gamma         = 1.4;
  constexpr double ShockPosition = 0.111;
  // A high-order candidate can reach vacuum at a shock before the limiter rejects it. The limiter
  // rejects every density up to DensityFloor.
  constexpr double DensityFloor = 1.0e-12;

  double momentumSquared(const double* const Q) {
    using Shortcuts = tutorials::exahype2::limiting::ADERDGSolver::Shortcuts;
    double result   = 0.0;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      result += Q[Shortcuts::j + dimension] * Q[Shortcuts::j + dimension];
    }
    return result;
  }
} // namespace

void
tutorials::exahype2::limiting::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  const bool   inLowPressureCorner = cellCentre[0] < -ShockPosition and cellCentre[1] < -ShockPosition;
  const double pressure            = inLowPressureCorner ? 0.1 : 1.0;

  Q[Shortcuts::rho]   = inLowPressureCorner ? 0.125 : 1.0;
  Q[Shortcuts::j + 0] = 0.0;
  Q[Shortcuts::j + 1] = 0.0;
  Q[Shortcuts::E]     = pressure / (Gamma - 1.0);
}

void
tutorials::exahype2::limiting::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                  Qinside,
  double* const NOALIAS                        Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const int                                    normal
) {
  // The outside state is the initial state at faceCentre for all times.
  const bool   inLowPressureCorner = faceCentre[0] < -ShockPosition and faceCentre[1] < -ShockPosition;
  const double pressure            = inLowPressureCorner ? 0.1 : 1.0;

  Qoutside[Shortcuts::rho]   = inLowPressureCorner ? 0.125 : 1.0;
  Qoutside[Shortcuts::j + 0] = 0.0;
  Qoutside[Shortcuts::j + 1] = 0.0;
  Qoutside[Shortcuts::E]     = pressure / (Gamma - 1.0);
}

void
tutorials::exahype2::limiting::ADERDGSolver::flux(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  double* const NOALIAS                        F
) {
  const double inverseDensity = 1.0 / std::max(Q[Shortcuts::rho], DensityFloor);
  const double pressure       = (Gamma - 1.0) * (Q[Shortcuts::E] - 0.5 * inverseDensity * momentumSquared(Q));
  const double normalMomentum = Q[Shortcuts::j + normal];

  F[Shortcuts::rho]   = normalMomentum;
  F[Shortcuts::j + 0] = normalMomentum * Q[Shortcuts::j + 0] * inverseDensity;
  F[Shortcuts::j + 1] = normalMomentum * Q[Shortcuts::j + 1] * inverseDensity;
  F[Shortcuts::E]     = normalMomentum * inverseDensity * (Q[Shortcuts::E] + pressure);

  F[Shortcuts::j + normal] += pressure;
}

double
tutorials::exahype2::limiting::ADERDGSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal
) {
  const double inverseDensity = 1.0 / std::max(Q[Shortcuts::rho], DensityFloor);
  const double pressure       = (Gamma - 1.0) * (Q[Shortcuts::E] - 0.5 * inverseDensity * momentumSquared(Q));
  // The solver computes the time step size before the limiter checks the cell. The eigenvalue of a
  // candidate with negative pressure has to stay finite.
  const double soundSpeed     = std::sqrt(std::abs(Gamma * pressure * inverseDensity));
  const double normalVelocity = Q[Shortcuts::j + normal] * inverseDensity;

  return std::abs(normalVelocity) + soundSpeed;
}
