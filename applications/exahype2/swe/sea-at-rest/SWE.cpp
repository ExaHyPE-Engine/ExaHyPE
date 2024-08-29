// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SWE.h"

::tarch::logging::Log applications::exahype2::swe::SWE::_log(
  "applications::exahype2::swe::SWE"
);

using s = applications::exahype2::swe::VariableShortcuts;

void applications::exahype2::swe::SWE::initialCondition(
  double* __restrict__ Q,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  bool                                           gridIsConstructed
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  const double domainSizeHalfX = DomainSize[0] / 2.0;

  Q[s::h] = x[0] <= domainSizeHalfX
              ? INITIAL_WATER_HEIGHT_LEFT_OF_DISCONTINUITY
              : INITIAL_WATER_HEIGHT_RIGHT_OF_DISCONTINUITY;
  Q[s::b] = -Q[s::h];
}


void applications::exahype2::swe::SWE::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  int                                            normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


::exahype2::RefinementCommand applications::exahype2::swe::SWE::
  refinementCriterion(
    const double* __restrict__ Q,
    const ::tarch::la::Vector<Dimensions, double>& x,
    const ::tarch::la::Vector<Dimensions, double>& h,
    double                                         t
  ) {
  auto result = ::exahype2::RefinementCommand::Keep;

  const double domainSizeHalfX = DomainSize[0] / 2.0;

  if (x(0) >= domainSizeHalfX - 1.0 && x(0) <= domainSizeHalfX + 1.0) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
}
