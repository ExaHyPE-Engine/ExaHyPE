// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// The a-posteriori limiter's troubled-cell test. After each step the limiter
// calls this for every cell; returning false marks the cell as troubled, so it
// is rolled back and recomputed with the Finite Volume scheme.
#include "LimitingSolver.h"

#include "ADERDGSolver.h"

bool tutorials::exahype2::limiting::LimitingSolver::isPhysicallyAdmissible(
  const double* const                          Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 timeStamp
) {
  // Q is the regular (ADER-DG) solver's state, so we reuse its variable shortcuts
  // and its NumberOfUnknowns constant instead of hardcoding indices or a count.
  using Shortcuts = ADERDGSolver::Shortcuts;

  constexpr double Gamma = 1.4;
  constexpr double pZero = 1.0e-4;
  const double irho = Q[Shortcuts::rho] > 0.0 ? 1.0 / Q[Shortcuts::rho] : 0.0;
  const double p    = (Gamma - 1.0)
    * (Q[Shortcuts::E] - 0.5 * irho * (Q[Shortcuts::j + 0] * Q[Shortcuts::j + 0] + Q[Shortcuts::j + 1] * Q[Shortcuts::j + 1]));

  // Density, energy and pressure must stay positive, and every value finite.
  if (p < pZero || Q[Shortcuts::rho] < 0.0 || Q[Shortcuts::E] < 0.0) {
    return false;
  }
  for (int i = 0; i < ADERDGSolver::NumberOfUnknowns; ++i) {
    if (!std::isfinite(Q[i])) {
      return false;
    }
  }
  return true;
}
