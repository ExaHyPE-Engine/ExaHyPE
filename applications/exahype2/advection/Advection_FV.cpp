// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Advection_FV.h"

tarch::logging::Log applications::exahype2::advection::Advection_FV::_log("applications::exahype2::advection::Advection_FV");

void applications::exahype2::advection::Advection_FV::initialCondition(
  double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, bool gridIsConstructed
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = x[i];
  }
}

void applications::exahype2::advection::Advection_FV::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  int                                          normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}

::exahype2::RefinementCommand applications::exahype2::advection::Advection_FV::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t
) {
  using s = VariableShortcuts;

  auto result = ::exahype2::RefinementCommand::Keep;

  if (x(0) > 0.5) {
    result = ::exahype2::RefinementCommand::Refine;
  } else {
    result = ::exahype2::RefinementCommand::Erase;
  }

  if constexpr (UseDynamicAMR) {
    if (tarch::la::greater(t, 0.0)) {
      const double u = Q[s::v];

      if (u >= 0.5) {
        result = ::exahype2::RefinementCommand::Refine;
      }
      if (u < 0.5) {
        result = ::exahype2::RefinementCommand::Erase;
      }
    }
  }

  return result;
}
