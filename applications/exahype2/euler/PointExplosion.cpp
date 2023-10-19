#include "EulerSolver.h"

void applications::exahype2::euler::EulerSolver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    index,
  bool                                         gridIsConstructred
) {
  Q[0] = 1.0;
  Q[1] = 0;
  Q[2] = 0;
#if Dimensions == 2
  Q[3] = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2)) < 0.2) ? (1.0) : (1.01));
#else
  Q[3] = 0;
  Q[4] = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2) + std::pow(0.5 - x(2), 2)) < 0.2) ? (1.0) : (1.01));
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

#if Dimensions == 3
  tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5, 0.5};
#else
  tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5};
#endif

  // This is an example how to implement a priori refinement.
  // If you only have this thing, then you work with static AMR,
  // as you never invoke erase.
  if (tarch::la::equals(t, 0.0)) {
    bool isInTheCentre = (tarch::la::norm2(x - circleCentre) < MaxAdmissibleCellH);
    if (isInTheCentre) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  if constexpr (UseDynamicAMR) {
    // This is an example how to implement dynamic AMR, as the
    // AMR instruction depends on the actual solution (which is
    // not directly available throughout the grid construction).
    if (tarch::la::greater(t, 0.0)) {
      const double density = Q[0];

      if (density >= 1.00134 && density < 1.0017) {
        result = ::exahype2::RefinementCommand::Refine;
      } else {
        result = ::exahype2::RefinementCommand::Erase;
      }
    }
  }

  return result;
}
