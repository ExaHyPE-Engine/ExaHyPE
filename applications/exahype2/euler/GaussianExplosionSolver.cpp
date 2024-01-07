// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GaussianExplosionSolver.h"

tarch::logging::Log applications::exahype2::euler::GaussianExplosionSolver::_log("applications::exahype2::euler::GaussianExplosionSolver");

using Vars = applications::exahype2::euler::VariableShortcuts;

static constexpr double CIRCLE_RADIUS = 0.2;
#if Dimensions == 3
// Manual offset to make the wave originate slightly to the left of the center --- helps
// to detect if wave is moving to the left or right.
const tarch::la::Vector<Dimensions, double> CIRCLE_CENTER = {0.18, 0.3, 0.6};
#else
const tarch::la::Vector<Dimensions, double> CIRCLE_CENTER = {0.5, 0.5};
#endif


void applications::exahype2::euler::GaussianExplosionSolver::initialCondition(
  double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, int>&, bool
) {
  const double peakDeviation = MaxAdmissibleCellH;
  double       distance      = tarch::la::norm2(x - circleCentre);
  double       exponent      = -(distance * distance) / 2.0 / peakDeviation / peakDeviation;

  Q[Vars::rho] = 0.1;
  Q[Vars::u]   = 0;
  Q[Vars::v]   = 0;

#if Dimensions == 2
  Q[Var::e] = std::exp(exponent);
#else
  Q[Vars::w] = 0;
  Q[Vars::e] = std::exp(exponent);
#endif
}


void applications::exahype2::euler::GaussianExplosionSolver::boundaryConditions(
  const double* __restrict__ Qinside, double* __restrict__ Qoutside, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, double>&, double, int
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


::exahype2::RefinementCommand applications::exahype2::euler::GaussianExplosionSolver::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t
) {
  if (tarch::la::equals(t, 0.0)) {
    if (tarch::la::norm2(x - CIRCLE_CENTER) < MaxAdmissibleCellH) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  if constexpr (USE_DYNAMIC_AMR) {
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
