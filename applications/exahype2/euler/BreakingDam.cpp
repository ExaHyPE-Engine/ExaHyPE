#include "EulerSolver.h"

void applications::exahype2::euler::EulerSolver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    index,
  bool                                         gridIsConstructred
) {
  Q[0] = 0.1;
  Q[1] = 0;
  Q[2] = 0;
#if Dimensions == 2
  Q[3] = ((x(0) < 0.5) ? (1.0) : (0));
#else
  Q[3] = 0;
  Q[4] = ((x(0) < 0.5) ? (1.0) : (0));
#endif
}

void applications::exahype2::euler::EulerSolver::boundaryConditions(
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

  /**
   * Apply Neumann BCs to all quantities. This includes the material
   * parameters (auxiliary ones).
   */
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}

::exahype2::RefinementCommand applications::exahype2::euler::EulerSolver::refinementCriterion(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t
) {
  auto result = ::exahype2::RefinementCommand::Keep;

  // This is an example how to implement a priori refinement.
  // If you only have this thing, then you work with static AMR,
  // as you never invoke erase.
  if (tarch::la::equals(t, 0.0)) {
    if (x(0) < 0.5) {
      result = ::exahype2::RefinementCommand::Refine;
    } else {
      result = ::exahype2::RefinementCommand::Erase;
    }
  }

  return result;
}
