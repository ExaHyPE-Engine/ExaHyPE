// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// The functions implement the compressible Euler equations for a 2D Riemann problem. The subcell
// scheme of a troubled cell evaluates the same functions.
//
// Q.rho contains the density, Q.j the momentum and Q.E the total energy.
#include "ADERDGSolver.h"

namespace {
  constexpr double Gamma         = 1.4;
  constexpr double ShockPosition = 0.111;
  // A high-order candidate can reach vacuum at a shock before the limiter rejects it. The limiter
  // rejects every density up to DensityFloor.
  constexpr double DensityFloor = 1.0e-12;

  double momentumSquared(const tutorials::exahype2::limiting::ADERDGSolver::Unknowns& Q) {
    double result = 0.0;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      result += Q.j(dimension) * Q.j(dimension);
    }
    return result;
  }
} // namespace

void
tutorials::exahype2::limiting::ADERDGSolver::initialCondition(
  State&                   Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const RuntimeParameters& parameters
) {
  const bool   inLowPressureCorner = cellCentre[0] < -ShockPosition and cellCentre[1] < -ShockPosition;
  const double pressure            = inLowPressureCorner ? 0.1 : 1.0;

  Q.rho = inLowPressureCorner ? 0.125 : 1.0;
  Q.j   = 0.0;
  Q.E   = pressure / (Gamma - 1.0);
}

void
tutorials::exahype2::limiting::ADERDGSolver::boundaryConditions(
  const State&             Qinside,
  State&                   Qoutside,
  const Coordinates&       faceCentre,
  const Coordinates&       cellSize,
  const double             time,
  const int                normal,
  const RuntimeParameters& parameters
) {
  // The outside state is the initial state at faceCentre for all times.
  const bool   inLowPressureCorner = faceCentre[0] < -ShockPosition and faceCentre[1] < -ShockPosition;
  const double pressure            = inLowPressureCorner ? 0.1 : 1.0;

  Qoutside.rho = inLowPressureCorner ? 0.125 : 1.0;
  Qoutside.j   = 0.0;
  Qoutside.E   = pressure / (Gamma - 1.0);
}

void
tutorials::exahype2::limiting::ADERDGSolver::flux(
  const State&             Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const double             time,
  const double             timeStepSize,
  const int                normal,
  Flux&                    F,
  const RuntimeParameters& parameters
) {
  const double inverseDensity = 1.0 / std::max(Q.rho, DensityFloor);
  const double pressure       = (Gamma - 1.0) * (Q.E - 0.5 * inverseDensity * momentumSquared(Q));
  const double normalMomentum = Q.j(normal);

  F.rho = normalMomentum;
  F.j   = normalMomentum * Q.j * inverseDensity;
  F.E   = normalMomentum * inverseDensity * (Q.E + pressure);

  F.j(normal) += pressure;
}

double
tutorials::exahype2::limiting::ADERDGSolver::maxEigenvalue(
  const State&             Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const double             time,
  const double             timeStepSize,
  const int                normal,
  const RuntimeParameters& parameters
) {
  const double inverseDensity = 1.0 / std::max(Q.rho, DensityFloor);
  const double pressure       = (Gamma - 1.0) * (Q.E - 0.5 * inverseDensity * momentumSquared(Q));
  // The solver computes the time step size before the limiter checks the cell. The eigenvalue of a
  // candidate with negative pressure has to stay finite.
  const double soundSpeed     = std::sqrt(std::abs(Gamma * pressure * inverseDensity));
  const double normalVelocity = Q.j(normal) * inverseDensity;

  return std::abs(normalVelocity) + soundSpeed;
}
