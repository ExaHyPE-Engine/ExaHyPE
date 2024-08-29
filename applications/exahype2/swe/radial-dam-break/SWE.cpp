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

  const double domainSizeHalfX    = DomainSize[0] / 2.0;
  const double domainSizeHalfY    = DomainSize[1] / 2.0;
  const double distanceFromOrigin = sqrt(
    (x[0] - domainSizeHalfX) * (x[0] - domainSizeHalfX)
    + (x[1] - domainSizeHalfY) * (x[1] - domainSizeHalfY)
  );
  Q[s::h] = distanceFromOrigin <= DAM_RADIUS
              ? INITIAL_WATER_HEIGHT_INSIDE_DAM
              : INITIAL_WATER_HEIGHT_OUTSIDE_DAM;
}


void applications::exahype2::swe::SWE::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  int                                            normal
) {
  Qoutside[s::h]      = Qinside[s::h];
  Qoutside[s::hu + 0] = -Qinside[s::hu + 0];
  Qoutside[s::hu + 1] = -Qinside[s::hu + 1];
  Qoutside[s::b]      = Qinside[s::b];
}


::exahype2::RefinementCommand applications::exahype2::swe::SWE::
  refinementCriterion(
    const double* __restrict__ Q,
    const ::tarch::la::Vector<Dimensions, double>& x,
    const ::tarch::la::Vector<Dimensions, double>& h,
    double                                         t
  ) {
  const double domainSizeHalfX    = DomainSize[0] / 2.0;
  const double domainSizeHalfY    = DomainSize[1] / 2.0;
  const double distanceFromOrigin = sqrt(
    (x[0] - domainSizeHalfX) * (x[0] - domainSizeHalfX)
    + (x[1] - domainSizeHalfY) * (x[1] - domainSizeHalfY)
  );

  return distanceFromOrigin <= DAM_RADIUS
           ? ::exahype2::RefinementCommand::Refine
           : ::exahype2::RefinementCommand::Keep;
}
