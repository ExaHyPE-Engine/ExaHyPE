// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Advection.h"

tarch::logging::Log applications::exahype2::advection::Advection::_log(
  "applications::exahype2::advection::Advection"
);

using s = applications::exahype2::advection::VariableShortcuts;

void applications::exahype2::advection::Advection::initialCondition(
  double* __restrict__                          Q,
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  bool                                          gridIsConstructed
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = std::sin(x(0) * tarch::la::PI) * std::sin(x(1) * tarch::la::PI)
    #if Dimensions == 3
      * std::sin(x(2) * tarch::la::PI)
    #endif
    ;
  }
}


void applications::exahype2::advection::Advection::boundaryConditions(
  const double* __restrict__                    Qinside,
  double* __restrict__                          Qoutside,
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  double                                        t,
  int                                           normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


::exahype2::RefinementCommand applications::exahype2::advection::Advection::
  refinementCriterion(
    const double* __restrict__                    Q,
    const tarch::la::Vector<Dimensions, double>&  x,
    const tarch::la::Vector<Dimensions, double>&  h,
    double                                        t
  ) {
  auto result = ::exahype2::RefinementCommand::Keep;

  if (x(0) > 0.5) {
    result = ::exahype2::RefinementCommand::Refine;
  } else {
    result = ::exahype2::RefinementCommand::Erase;
  }

  return result;
}
