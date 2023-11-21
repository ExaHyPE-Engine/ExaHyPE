// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AcousticWave_ADERDG.h"

tarch::logging::Log applications::exahype2::acousticwave::AcousticWave_ADERDG::_log("applications::exahype2::acousticwave::AcousticWave_ADERDG");

void applications::exahype2::acousticwave::AcousticWave_ADERDG::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    index,
  bool                                         gridIsConstructed
) {
  using s = VariableShortcuts;

  Q[s::p]     = 0.0;
  Q[s::v + 0] = 0.0;
  Q[s::v + 1] = 0.0;
#if Dimensions == 3
  Q[s::v + 2] = 0.0;
#endif
}

void applications::exahype2::acousticwave::AcousticWave_ADERDG::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  int                                          normal
) {
  using s = VariableShortcuts;

  Qoutside[s::p]     = 0.0;
  Qoutside[s::v + 0] = 0.0;
  Qoutside[s::v + 1] = 0.0;
#if Dimensions == 3
  Qoutside[s::v + 2] = 0.0;
#endif
}

::exahype2::RefinementCommand applications::exahype2::acousticwave::AcousticWave_ADERDG::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t
) {
  using s = VariableShortcuts;

  auto result = ::exahype2::RefinementCommand::Keep;

#if Dimensions == 3
  tarch::la::Vector<Dimensions, double> circleCentre = {5, 5, 5};
#else
  tarch::la::Vector<Dimensions, double> circleCentre = {5, 5};
#endif

  // This is an example how to implement a priori refinement.
  // If you only have this thing, then you work with static AMR,
  // as you never invoke erase.
  if (tarch::la::equals(t, 0.0)) {
    bool isInTheCentre = (tarch::la::norm2(x - circleCentre) < 1.0);
    if (isInTheCentre) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  if constexpr (UseDynamicAMR) {
    if (tarch::la::greater(t, 0.0)) {
      // This is an example how to implement dynamic AMR, as the
      // AMR instruction depends on the actual solution (which is
      // not directly available throughout the grid construction).
      // If you remove this part, you get static AMR.
      const double pressure = Q[s::p];

      if (pressure >= 0.05) {
        result = ::exahype2::RefinementCommand::Refine;
      }
      if (pressure < 0.05) {
        result = ::exahype2::RefinementCommand::Erase;
      }
    }
  }

  return result;
}
