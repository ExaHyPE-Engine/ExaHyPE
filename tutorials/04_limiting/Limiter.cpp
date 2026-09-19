// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

namespace {
  constexpr double Gamma = 1.4;
  // The Euler equations describe a gas only while density and pressure are
  // strictly positive, so a candidate that reaches either floor goes to the
  // subcell scheme.
  constexpr double DensityFloor  = 1.0e-12;
  constexpr double PressureFloor = 1.0e-12;

  // Positions in the observable vector the discrete maximum principle bounds.
  constexpr int Density  = 0;
  constexpr int Velocity = 1;
  constexpr int Pressure = 1 + DIMENSIONS;

  double momentumSquared(const double* const Q) {
    using Shortcuts = tutorials::exahype2::limiting::ADERDGSolver::Shortcuts;
    double result   = 0.0;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      result += Q[Shortcuts::j + dimension] * Q[Shortcuts::j + dimension];
    }
    return result;
  }
} // namespace

bool tutorials::exahype2::limiting::ADERDGSolver::isPhysicallyAdmissible(
  const double* const NOALIAS                  Q,
  const int                                    numberOfStates,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  constexpr int NumberOfData = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  // Q holds numberOfStates consecutive states: Every node of a candidate
  // polynomial, or a single state when one finite-volume value is checked.
  const double* state = Q;
  for (int stateNumber = 0; stateNumber < numberOfStates; ++stateNumber, state += NumberOfData) {
    for (int unknown = 0; unknown < NumberOfData; ++unknown) {
      if (not std::isfinite(state[unknown])) {
        return false;
      }
    }

    const double density = state[Shortcuts::rho];
    if (density <= DensityFloor) {
      return false;
    }

    const double pressure = (Gamma - 1.0) * (state[Shortcuts::E] - 0.5 * momentumSquared(state) / density);
    if (pressure <= PressureFloor) {
      return false;
    }
  }

  return true;
}

void tutorials::exahype2::limiting::ADERDGSolver::mapDmpObservables(
  double* const NOALIAS       observables,
  const double* const NOALIAS Q
) {
  static_assert(
    NumberOfDMPObservables == Pressure + 1,
    "The mapping fills the density, one velocity component per axis and the pressure."
  );

  const double inverseDensity = 1.0 / std::max(Q[Shortcuts::rho], DensityFloor);

  observables[Density] = Q[Shortcuts::rho];
  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    observables[Velocity + dimension] = Q[Shortcuts::j + dimension] * inverseDensity;
  }
  observables[Pressure] = (Gamma - 1.0) * (Q[Shortcuts::E] - 0.5 * inverseDensity * momentumSquared(Q));
}
