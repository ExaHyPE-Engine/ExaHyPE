// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ElasticWaveSolver.h"

tarch::logging::Log applications::exahype2::elastic::ElasticWaveSolver::_log("applications::exahype2::elastic::ElasticWaveSolver");

using Vars = applications::exahype2::elastic::VariableShortcuts;


void applications::exahype2::elastic::ElasticWaveSolver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    index,
  bool                                         gridIsConstructed
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
}


void applications::exahype2::elastic::ElasticWaveSolver::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  int                                          normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = 0.0;
  }
}


#ifdef USE_POINT_SOURCE
void applications::exahype2::elastic::ElasticWaveSolver::pointSource(
  const double* const Q, const double* const x, const double t, const double dt, double* const forceVector, int n
) {
  for (int i = 0; i < NumberOfUnknowns; i++) {
    forceVector[i] = 0.0;
  }

  forceVector[Vars::sxy] = SOURCE_M0 * t / (SOURCE_t0 * SOURCE_t0) * std::exp(-t / SOURCE_t0);
}


void applications::exahype2::elastic::ElasticWaveSolver::initPointSourceLocations(double sourceLocations[NumberOfPointSources][Dimensions]) {
  sourceLocations[0][0] = SOURCE_LOCATION(0);
  sourceLocations[0][1] = SOURCE_LOCATION(1);
  sourceLocations[0][2] = SOURCE_LOCATION(2);
}
#endif


::exahype2::RefinementCommand applications::exahype2::elastic::ElasticWaveSolver::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t
) {
  auto result = ::exahype2::RefinementCommand::Keep;

  if (tarch::la::equals(t, 0.0)) {
    if (tarch::la::norm2(x - SOURCE_LOCATION) < SOURCE_RADIUS) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  if constexpr (USE_DYNAMIC_AMR) {
    if (tarch::la::greater(t, 0.0)) {
      const double stress = Q[Vars::sxy];
      if (stress >= 5) {
        result = ::exahype2::RefinementCommand::Refine;
      } else if (stress < 5) {
        result = ::exahype2::RefinementCommand::Erase;
      }
    }
  }

  return result;
}
