// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Euler.h"

tarch::logging::Log applications::exahype2::euler::Euler::_log(
  "applications::exahype2::euler::Euler"
);

using s = applications::exahype2::euler::VariableShortcuts;

void applications::exahype2::euler::Euler::initialCondition(
  double* __restrict__                          Q,
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  bool                                          gridIsConstructed
) {
  Q[s::rho] = 0.1;
  Q[s::u]   = 0.0;
  Q[s::v]   = 0.0;
#if Dimensions == 3
  Q[s::w] = 0.0;
#endif
  Q[s::e] = ((x(0) < 0.5) ? (1.0) : (0));
}


void applications::exahype2::euler::Euler::boundaryConditions(
  const double* __restrict__                    Qinside,
  double* __restrict__                          Qoutside,
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  double                                        t,
  int                                           normal
) {
#ifdef GPUOffloadingOff
  nonCriticalAssertion4(Qinside[s::rho] > 1e-12, x, t, normal, Qinside[s::rho]);
#endif

  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


::exahype2::RefinementCommand applications::exahype2::euler::Euler::
  refinementCriterion(
    const double* __restrict__                    Q,
    const tarch::la::Vector<Dimensions, double>&  x,
    const tarch::la::Vector<Dimensions, double>&  h,
    double                                        t
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
