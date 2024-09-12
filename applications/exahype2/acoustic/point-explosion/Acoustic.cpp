// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Acoustic.h"

tarch::logging::Log applications::exahype2::acoustic::Acoustic::_log(
  "applications::exahype2::acoustic::Acoustic"
);

using s = applications::exahype2::acoustic::VariableShortcuts;

void applications::exahype2::acoustic::Acoustic::initialCondition(
  double* __restrict__                          Q,
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  bool                                          gridIsConstructed
) {
  Q[s::p] = 0.0;
  Q[s::u] = 0.0;
  Q[s::v] = 0.0;
#if Dimensions == 3
  Q[s::w] = 0.0;
#endif
}


void applications::exahype2::acoustic::Acoustic::boundaryConditions(
  const double* __restrict__                    Qinside,
  double* __restrict__                          Qoutside,
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  double                                        t,
  int                                           normal
) {
  Qoutside[s::p] = 0.0;
  Qoutside[s::u] = 0.0;
  Qoutside[s::v] = 0.0;
#if Dimensions == 3
  Qoutside[s::w] = 0.0;
#endif
}


::exahype2::RefinementCommand applications::exahype2::acoustic::Acoustic::
  refinementCriterion(
    const double* __restrict__                    Q,
    const tarch::la::Vector<Dimensions, double>&  x,
    const tarch::la::Vector<Dimensions, double>&  h,
    double                                        t
  ) {
  auto result = ::exahype2::RefinementCommand::Keep;

#if Dimensions == 3
  tarch::la::Vector<Dimensions, double> circleCentre = {5.0, 5.0, 5.0};
#else
  tarch::la::Vector<Dimensions, double> circleCentre = {5.0, 5.0};
#endif

  if (tarch::la::equals(t, 0.0)) {
    if (tarch::la::norm2(x - circleCentre) < 1.0) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  return result;
}
