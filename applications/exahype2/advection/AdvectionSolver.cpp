// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AdvectionSolver.h"

tarch::logging::Log applications::exahype2::advection::AdvectionSolver::_log("applications::exahype2::advection::AdvectionSolver");

using Vars = applications::exahype2::advection::VariableShortcuts;


void applications::exahype2::advection::AdvectionSolver::initialCondition(
  double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, int>&, bool
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = x[i];
  }
}


void applications::exahype2::advection::AdvectionSolver::boundaryConditions(
  const double* __restrict__ Qinside, double* __restrict__ Qoutside, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, double>&, double, int
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


::exahype2::RefinementCommand applications::exahype2::advection::AdvectionSolver::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>&, double t
) {
  auto result = ::exahype2::RefinementCommand::Keep;

  if (tarch::la::equals(t, 0.0)) {
    if (x(0) > 0.5) {
      result = ::exahype2::RefinementCommand::Refine;
    } else {
      result = ::exahype2::RefinementCommand::Erase;
    }
  }

  if constexpr (USE_DYNAMIC_AMR) {
    if (tarch::la::greater(t, 0.0)) {
      const double u = Q[Vars::u];
      if (u >= 0.5) {
        result = ::exahype2::RefinementCommand::Refine;
      } else if (u < 0.5) {
        result = ::exahype2::RefinementCommand::Erase;
      }
    }
  }

  return result;
}
