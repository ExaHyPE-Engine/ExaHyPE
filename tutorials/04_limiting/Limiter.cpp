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

  double momentumSquared(const double* const Q) {
    using Shortcuts = tutorials::exahype2::limiting::ADERDGSolver::Shortcuts;
    double result   = 0.0;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      result += Q[Shortcuts::j + dimension] * Q[Shortcuts::j + dimension];
    }
    return result;
  }
} // namespace

bool
tutorials::exahype2::limiting::ADERDGSolver::isPhysicallyAdmissible(
  const double* const NOALIAS                  Q,
  const int                                    numberOfStates,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  constexpr int NumberOfData = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  // Q stores numberOfStates consecutive states: The nodes or the subcells of a candidate, or one
  // subcell.
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

void
tutorials::exahype2::limiting::ADERDGSolver::mapDmpObservables(
  double* const NOALIAS       observables,
  const double* const NOALIAS Q
) {
  static_assert(
    NumberOfObservables == Pressure + 1,
    "The limiting tutorial expects number_of_dmp_observables = 4 in 2D and 5 in 3D."
  );

  const double inverseDensity = 1.0 / std::max(Q[Shortcuts::rho], DensityFloor);

  observables[Density] = Q[Shortcuts::rho];
  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    observables[Velocity + dimension] = Q[Shortcuts::j + dimension] * inverseDensity;
  }
  observables[Pressure] = (Gamma - 1.0) * (Q[Shortcuts::E] - 0.5 * inverseDensity * momentumSquared(Q));
}
