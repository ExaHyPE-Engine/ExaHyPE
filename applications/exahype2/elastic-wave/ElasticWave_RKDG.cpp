// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ElasticWave_RKDG.h"

tarch::logging::Log applications::exahype2::elasticwave::ElasticWave_RKDG::_log("applications::exahype2::elasticwave::ElasticWave_RKDG");

void applications::exahype2::elasticwave::ElasticWave_RKDG::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    index,
  bool                                         gridIsConstructed
) {
  using s = VariableShortcuts;

  Q[s::v + 0]     = 0.0;
  Q[s::v + 1]     = 0.0;
  Q[s::sigma + 0] = 0.0;
  Q[s::sigma + 1] = 0.0;
  Q[s::sigma + 2] = 0.0;
}

void applications::exahype2::elasticwave::ElasticWave_RKDG::boundaryConditions(
  const double* __restrict__ Qinside, double* __restrict__ Qoutside, const tarch::la::Vector<Dimensions, double>& x, double t, int normal
) {
  using s = VariableShortcuts;

  Qoutside[s::v + 0]     = 0.0;
  Qoutside[s::v + 1]     = 0.0;
  Qoutside[s::sigma + 0] = 0.0;
  Qoutside[s::sigma + 1] = 0.0;
  Qoutside[s::sigma + 2] = 0.0;
}

::exahype2::RefinementCommand applications::exahype2::elasticwave::ElasticWave_RKDG::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t
) {
  using s = VariableShortcuts;

  auto result = ::exahype2::RefinementCommand::Keep;

  if (tarch::la::equals(t, 0.0)) {
    const bool pointSource = tarch::la::norm2(x - tarch::la::Vector<Dimensions, double>({10, 10})) <= 1.0;
    if (pointSource) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  if constexpr (UseDynamicAMR) {
    if (tarch::la::greater(t, 0.0)) {
      const double stress = Q[s::sigma + 2];
      if (stress >= 5) {
        result = ::exahype2::RefinementCommand::Refine;
      }
      if (stress < 5) {
        result = ::exahype2::RefinementCommand::Erase;
      }
    }
  }

  return result;
}
