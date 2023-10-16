#include "AdvectionSolver.h"

void applications::exahype2::advection::AdvectionSolver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    index,
  bool                                         gridIsConstructred
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = x[i];
  }
}

void applications::exahype2::advection::AdvectionSolver::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  int                                          normal
) {
#ifdef GPUOffloadingOff
  nonCriticalAssertion4(Qinside[0] > 1e-12, x, t, normal, Qinside[0]);
#endif

  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}

::exahype2::RefinementCommand applications::exahype2::advection::AdvectionSolver::refinementCriterion(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t
) {
  auto result = ::exahype2::RefinementCommand::Keep;

  if (x(0) > 0.5) {
    result = ::exahype2::RefinementCommand::Refine;
  } else {
    result = ::exahype2::RefinementCommand::Erase;
  }

  if constexpr (UseDynamicAMR) {
    if (tarch::la::greater(t, 0.0)) {
      const double u = Q[0];

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
