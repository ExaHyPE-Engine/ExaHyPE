// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Elastic.h"

tarch::logging::Log applications::exahype2::elastic::Elastic::_log(
  "applications::exahype2::elastic::Elastic"
);

using s = applications::exahype2::elastic::VariableShortcuts;

void applications::exahype2::elastic::Elastic::initialCondition(
  double* __restrict__                          Q,
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  bool                                          gridIsConstructed
) {
  Q[s::v + 0]     = 0.0;
  Q[s::v + 1]     = 0.0;
  Q[s::sigma + 0] = 0.0;
  Q[s::sigma + 1] = 0.0;
  Q[s::sigma + 2] = 0.0;
}


void applications::exahype2::elastic::Elastic::boundaryConditions(
  const double* __restrict__                    Qinside,
  double* __restrict__                          Qoutside,
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  double                                        t,
  int                                           normal
) {
  Qoutside[s::v + 0]     = 0.0;
  Qoutside[s::v + 1]     = 0.0;
  Qoutside[s::sigma + 0] = 0.0;
  Qoutside[s::sigma + 1] = 0.0;
  Qoutside[s::sigma + 2] = 0.0;
}


::exahype2::RefinementCommand applications::exahype2::elastic::Elastic::
  refinementCriterion(
    const double* __restrict__                    Q,
    const tarch::la::Vector<Dimensions, double>&  x,
    const tarch::la::Vector<Dimensions, double>&  h,
    double                                        t
  ) {
  auto result = ::exahype2::RefinementCommand::Keep;

#if Dimensions == 3
  tarch::la::Vector<Dimensions, double> circleCentre = {10.0, 10.0, 10.0};
#else
  tarch::la::Vector<Dimensions, double> circleCentre = {10.0, 10.0};
#endif

  if (tarch::la::equals(t, 0.0)) {
    if (tarch::la::norm2(x - circleCentre) < 1.0) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  return result;
}
