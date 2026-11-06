// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// The function bodies are compilable stubs of the exercises in ShallowWater.ipynb. With the stubs,
// Q keeps its initial state. The documentation pages "Part 2 - Solution to Exercise 1" and
// "Part 2 - Solution to Exercise 2" contain the solutions.
//
// Q stores the water height at Shortcuts::h, the momentum h*u at Shortcuts::hu + 0 and
// Shortcuts::hu + 1, and the bathymetry at Shortcuts::b.
#include "ADERDGSolver.h"

void
tutorials::exahype2::swe::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const RuntimeParameters&                     parameters
) {
  // TODO Exercise 1: Set up the dam-break initial state.
  // For now we start from flat water at rest so the code runs.
  Q[Shortcuts::h]      = 1.0;
  Q[Shortcuts::hu + 0] = 0.0;
  Q[Shortcuts::hu + 1] = 0.0;
  Q[Shortcuts::b]      = 0.0;
}

void
tutorials::exahype2::swe::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                  Qinside,
  double* const NOALIAS                        Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const int                                    normal,
  const RuntimeParameters&                     parameters
) {
  // TODO Exercise 1: Prescribe the outside state. As a safe default we mirror
  // the inside state (a crude outflow condition).
  Qoutside[Shortcuts::h]      = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu + 0] = Qinside[Shortcuts::hu + 0];
  Qoutside[Shortcuts::hu + 1] = Qinside[Shortcuts::hu + 1];
  Qoutside[Shortcuts::b]      = Qinside[Shortcuts::b];
}

::exahype2::RefinementCommand
tutorials::exahype2::swe::ADERDGSolver::refinementCriterion(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const RuntimeParameters&                     parameters
) {
  // TODO Exercise 2: Refine near the moving front. Keep the mesh as is for now.
  return ::exahype2::RefinementCommand::Keep;
}

double
tutorials::exahype2::swe::ADERDGSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  const RuntimeParameters&                     parameters
) {
  // TODO Exercise 1: Return max(|u_n + sqrt(g h)|, |u_n - sqrt(g h)|),
  // where u_n = hu[normal] / h and g = 9.81. A positive placeholder keeps the
  // time-step computation well defined until you implement the real value.
  return 1.0;
}

void
tutorials::exahype2::swe::ADERDGSolver::flux(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  double* const NOALIAS                        F,
  const RuntimeParameters&                     parameters
) {
  // TODO Exercise 1: Implement the shallow-water flux in direction `normal`.
  F[Shortcuts::h]      = 0.0;
  F[Shortcuts::hu + 0] = 0.0;
  F[Shortcuts::hu + 1] = 0.0;
  F[Shortcuts::b]      = 0.0;
}

void
tutorials::exahype2::swe::ADERDGSolver::nonconservativeProduct(
  const double* const NOALIAS                  Q,
  const double* const NOALIAS                  deltaQ,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  double* const NOALIAS                        BTimesDeltaQ,
  const RuntimeParameters&                     parameters
) {
  // TODO Exercise 2: Implement the bathymetry term B(Q)·∇Q. Zero for now.
  BTimesDeltaQ[Shortcuts::h]      = 0.0;
  BTimesDeltaQ[Shortcuts::hu + 0] = 0.0;
  BTimesDeltaQ[Shortcuts::hu + 1] = 0.0;
  BTimesDeltaQ[Shortcuts::b]      = 0.0;
}
