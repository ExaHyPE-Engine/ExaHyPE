// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Euler_ADERDG.h"

tarch::logging::Log applications::exahype2::euler::Euler_ADERDG::_log("applications::exahype2::euler::Euler_ADERDG");

void applications::exahype2::euler::Euler_ADERDG::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    index,
  bool                                         gridIsConstructed
) {
  using s = VariableShortcuts;

#if SCENARIO == PointExplosion
  Q[s::rho]   = 1.0;
  Q[s::v + 0] = 0;
  Q[s::v + 1] = 0;
#if Dimensions == 2
  Q[s::e] = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2)) < 0.2) ? (1.0) : (1.01));
#else
  Q[s::v + 2]                                        = 0;
  Q[s::e]                                            = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2) + std::pow(0.5 - x(2), 2)) < 0.2) ? (1.0) : (1.01));
#endif
#elif SCENARIO == GaussianExplosion
  // Manual offset to make the wave originate slightly to the left of the center --- helps
  // to detect if wave is moving to the left or right.
#if Dimensions == 2
  const tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.3};
#else
  const tarch::la::Vector<Dimensions, double> circleCentre = {0.18, 0.3, 0.6};
#endif

  const double peakDeviation = MaxAdmissibleCellH;
  double       distance      = tarch::la::norm2(x - circleCentre);
  double       exponent      = -(distance * distance) / 2.0 / peakDeviation / peakDeviation;

  Q[s::rho]   = 0.1;                // rho
  Q[s::v + 0] = 0;                  // u
  Q[s::v + 1] = 0;                  // v

#if Dimensions == 2
  Q[s::e]     = std::exp(exponent); // e
#else
  Q[s::v + 2]                                              = 0;                  // w
  Q[s::e]                                                  = std::exp(exponent); // e
#endif
#elif SCENARIO == BreakingDam
  Q[s::rho]   = 0.1;
  Q[s::v + 0] = 0;
  Q[s::v + 1] = 0;
#if Dimensions == 2
  Q[s::e]     = ((x(0) < 0.5) ? (1.0) : (0));
#else
  Q[s::v + 2] = 0;
  Q[s::e]     = ((x(0) < 0.5) ? (1.0) : (0));
#endif
#endif
}

void applications::exahype2::euler::Euler_ADERDG::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  int                                          normal
) {
  using s = VariableShortcuts;

#ifdef GPUOffloadingOff
  nonCriticalAssertion4(Qinside[s::rho] > 1e-12, x, t, normal, Qinside[s::rho]);
#endif

#if SCENARIO == PointExplosion
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
#elif SCENARIO == GaussianExplosion
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
#elif SCENARIO == BreakingDam
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
#endif
}

::exahype2::RefinementCommand applications::exahype2::euler::Euler_ADERDG::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t
) {
  using s = VariableShortcuts;

  auto result = ::exahype2::RefinementCommand::Keep;

#if SCENARIO == PointExplosion
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
      const double density = Q[s::rho];

      if (density >= 1.00134 && density < 1.0017) {
        result = ::exahype2::RefinementCommand::Refine;
      } else {
        result = ::exahype2::RefinementCommand::Erase;
      }
    }
  }
#elif SCENARIO == GaussianExplosion
#if Dimensions == 3
  tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5, 0.5};
#else
  tarch::la::Vector<Dimensions, double> circleCentre       = {0.5, 0.5};
#endif

  if (tarch::la::equals(t, 0.0)) {
    bool isInTheCentre = (tarch::la::norm2(x - circleCentre) < MaxAdmissibleCellH);
    if (isInTheCentre) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  if constexpr (UseDynamicAMR) {
    if (tarch::la::greater(t, 0.0)) {
      const double density = Q[s::rho];

      if (density >= 1.00134 && density < 1.0017) {
        result = ::exahype2::RefinementCommand::Refine;
      } else {
        result = ::exahype2::RefinementCommand::Erase;
      }
    }
  }
#elif SCENARIO == BreakingDam
  if (tarch::la::equals(t, 0.0)) {
    if (x(0) < 0.5) {
      result = ::exahype2::RefinementCommand::Refine;
    } else {
      result = ::exahype2::RefinementCommand::Erase;
    }
  }
#endif

  return result;
}
