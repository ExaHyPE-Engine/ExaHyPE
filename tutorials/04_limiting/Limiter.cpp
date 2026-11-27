// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

namespace {
  constexpr double Gamma = 1.4;
  // isPhysicallyAdmissible() rejects a state with a density or a pressure at or below its floor.
  // SubcellScheme then recomputes the candidate.
  constexpr double DensityFloor  = 1.0e-12;
  constexpr double PressureFloor = 1.0e-12;

  constexpr int Density  = 0;
  constexpr int Velocity = 1;
  constexpr int Pressure = 1 + DIMENSIONS;

  double momentumSquared(const tutorials::exahype2::limiting::ADERDGSolver::Unknowns& Q) {
    double result = 0.0;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      result += Q.j(dimension) * Q.j(dimension);
    }
    return result;
  }
} // namespace

bool
tutorials::exahype2::limiting::ADERDGSolver::isPhysicallyAdmissible(
  const States             Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const RuntimeParameters& parameters
) {
  // Q contains the nodes or the subcells of a candidate, or one subcell.
  for (const auto& state : Q) {
    if (not allOf(state, [](const double value) { return std::isfinite(value); })) {
      return false;
    }

    const double density = state.rho;
    if (density <= DensityFloor) {
      return false;
    }

    const double pressure = (Gamma - 1.0) * (state.E - 0.5 * momentumSquared(state) / density);
    if (pressure <= PressureFloor) {
      return false;
    }
  }

  return true;
}

void
tutorials::exahype2::limiting::ADERDGSolver::mapDmpObservables(
  Observables&             observables,
  const State&             Q,
  const RuntimeParameters& parameters
) {
  static_assert(
    NumberOfObservables == Pressure + 1,
    "The limiting tutorial expects number_of_dmp_observables = 4 in 2D and 5 in 3D."
  );

  const double inverseDensity = 1.0 / std::max(Q.rho, DensityFloor);

  observables(Density) = Q.rho;
  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    observables(Velocity + dimension) = Q.j(dimension) * inverseDensity;
  }
  observables(Pressure) = (Gamma - 1.0) * (Q.E - 0.5 * inverseDensity * momentumSquared(Q));
}
