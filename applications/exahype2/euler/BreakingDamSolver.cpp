// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "BreakingDamSolver.h"

tarch::logging::Log applications::exahype2::euler::BreakingDamSolver::_log("applications::exahype2::euler::BreakingDamSolver");

using Vars = applications::exahype2::euler::VariableShortcuts;


void applications::exahype2::euler::BreakingDamSolver::initialCondition(
  double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, int>&, bool
) {
  Q[Vars::rho] = 0.1;
  Q[Vars::u]   = 0;
  Q[Vars::v]   = 0;
#if Dimensions == 2
  Q[Vars::e] = ((x(0) < 0.5) ? (1.0) : (0));
#else
  Q[Vars::w] = 0;
  Q[Vars::e] = ((x(0) < 0.5) ? (1.0) : (0));
#endif
}


void applications::exahype2::euler::BreakingDamSolver::boundaryConditions(
  const double* __restrict__ Qinside, double* __restrict__ Qoutside, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, double>&, double, int
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


::exahype2::RefinementCommand applications::exahype2::euler::BreakingDamSolver::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t
) {
  auto result = ::exahype2::RefinementCommand::Keep;

  if (tarch::la::equals(t, 0.0)) {
    if (x(0) < 0.5) {
      result = ::exahype2::RefinementCommand::Refine;
    } else {
      result = ::exahype2::RefinementCommand::Erase;
    }
  }

  return result;
}
