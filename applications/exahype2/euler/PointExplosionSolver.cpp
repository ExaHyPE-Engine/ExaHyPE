// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PointExplosionSolver.h"

tarch::logging::Log applications::exahype2::euler::PointExplosionSolver::_log("applications::exahype2::euler::PointExplosionSolver");

using Vars = applications::exahype2::euler::VariableShortcuts;

static constexpr double CIRCLE_RADIUS = 0.2;
#if Dimensions == 3
const tarch::la::Vector<Dimensions, double> CIRCLE_CENTER = {0.5, 0.5, 0.5};
#else
const tarch::la::Vector<Dimensions, double> CIRCLE_CENTER = {0.5, 0.5};
#endif


void applications::exahype2::euler::PointExplosionSolver::initialCondition(
  double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, int>&, bool
) {
  Q[Vars::rho] = 1.0;
  Q[Vars::u]   = 0;
  Q[Vars::v]   = 0;
#if Dimensions == 2
  Q[Vars::e] = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2)) < CIRCLE_RADIUS) ? (1.0) : (1.01));
#else
  Q[Vars::w] = 0;
  Q[Vars::e] = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2) + std::pow(0.5 - x(2), 2)) < CIRCLE_RADIUS) ? (1.0) : (1.01));
#endif
}


void applications::exahype2::euler::PointExplosionSolver::boundaryConditions(
  const double* __restrict__ Qinside, double* __restrict__ Qoutside, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, double>&, double, int
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


::exahype2::RefinementCommand applications::exahype2::euler::PointExplosionSolver::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t
) {
  auto result = ::exahype2::RefinementCommand::Keep;

  // This is an example how to implement a priori refinement.
  // If you only have this thing, then you work with static AMR,
  // as you never invoke erase.
  if (tarch::la::equals(t, 0.0)) {
    if (tarch::la::norm2(x - CIRCLE_CENTER) < CIRCLE_RADIUS) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  if constexpr (USE_DYNAMIC_AMR) {
    // This is an example how to implement dynamic AMR, as the
    // AMR instruction depends on the actual solution (which is
    // not directly available throughout the grid construction).
    if (tarch::la::greater(t, 0.0)) {
      const double density = Q[Vars::rho];
      if (density >= 1.00134 && density < 1.0017) {
        result = ::exahype2::RefinementCommand::Refine;
      } else {
        result = ::exahype2::RefinementCommand::Erase;
      }
    }
  }

  return result;
}
