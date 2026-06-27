// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// This is the *exercise* implementation of the shallow-water solver. The bodies
// below are intentionally left as stubs: the project compiles and runs as is,
// but the simulation does nothing physical until you fill in the TODOs. Work
// through the steps in ShallowWater.ipynb. Worked solutions are given in the
// documentation (pages "Part 2 - Solution to Exercise 1" and
// "Part 2 - Solution to Exercise 2").
//
// The conserved variables are addressed through the generated Shortcuts enum:
//   Shortcuts::h        water height          (1 component)
//   Shortcuts::hu       momentum h*u          (2 components: +0 is x, +1 is y)
//   Shortcuts::b        bathymetry            (1 component)
#include "ADERDGSolver.h"

void tutorials::exahype2::swe::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  // TODO Exercise 1: set up the dam-break initial state.
  // For now we start from flat water at rest so the code runs.
  Q[Shortcuts::h]      = 1.0;
  Q[Shortcuts::hu + 0] = 0.0;
  Q[Shortcuts::hu + 1] = 0.0;
  Q[Shortcuts::b]      = 0.0;
}

void tutorials::exahype2::swe::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                  Qinside,
  double* const NOALIAS                        Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const int                                    normal
) {
  // TODO Exercise 1: prescribe the outside state. As a safe default we mirror
  // the inside state (a crude outflow condition).
  Qoutside[Shortcuts::h]      = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu + 0] = Qinside[Shortcuts::hu + 0];
  Qoutside[Shortcuts::hu + 1] = Qinside[Shortcuts::hu + 1];
  Qoutside[Shortcuts::b]      = Qinside[Shortcuts::b];
}

::exahype2::RefinementCommand tutorials::exahype2::swe::ADERDGSolver::refinementCriterion(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time
) {
  // TODO Exercise 2: refine near the moving front. Keep the mesh as is for now.
  return ::exahype2::RefinementCommand::Keep;
}

double tutorials::exahype2::swe::ADERDGSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal
) {
  // TODO Exercise 1: return max(|u_n + sqrt(g h)|, |u_n - sqrt(g h)|),
  // where u_n = hu[normal] / h and g = 9.81. A positive placeholder keeps the
  // time-step computation well defined until you implement the real value.
  return 1.0;
}

void tutorials::exahype2::swe::ADERDGSolver::flux(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  double* const NOALIAS                        F
) {
  // TODO Exercise 1: implement the shallow-water flux in direction `normal`.
  F[Shortcuts::h]      = 0.0;
  F[Shortcuts::hu + 0] = 0.0;
  F[Shortcuts::hu + 1] = 0.0;
  F[Shortcuts::b]      = 0.0;
}

void tutorials::exahype2::swe::ADERDGSolver::nonconservativeProduct(
  const double* const NOALIAS                  Q,
  const double* const NOALIAS                  deltaQ,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  double* const NOALIAS                        BTimesDeltaQ
) {
  // TODO Exercise 2: implement the bathymetry term B(Q)·∇Q. Zero for now.
  BTimesDeltaQ[Shortcuts::h]      = 0.0;
  BTimesDeltaQ[Shortcuts::hu + 0] = 0.0;
  BTimesDeltaQ[Shortcuts::hu + 1] = 0.0;
  BTimesDeltaQ[Shortcuts::b]      = 0.0;
}
