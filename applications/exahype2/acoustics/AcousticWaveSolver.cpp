// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AcousticWaveSolver.h"

tarch::logging::Log applications::exahype2::acoustics::AcousticWaveSolver::_log("applications::exahype2::acoustics::AcousticWaveSolver");

using Vars = applications::exahype2::acoustics::VariableShortcuts;


void applications::exahype2::acoustics::AcousticWaveSolver::initialCondition(
  double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, int>&, bool
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
}


void applications::exahype2::acoustics::AcousticWaveSolver::boundaryConditions(
  const double* __restrict__, double* __restrict__ Qoutside, const tarch::la::Vector<Dimensions, double>&, const tarch::la::Vector<Dimensions, double>&, double, int
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = 0.0;
  }
}


#ifdef USE_POINT_SOURCES
void applications::exahype2::acoustics::AcousticWaveSolver::pointSource(
  const double* const Q, const double* const x, const double t, const double dt, double* const forceVector, int n
) {
  for (int i = 0; i < NumberOfUnknowns; i++) {
    forceVector[i] = 0.0;
  }

  forceVector[Vars::p] = SOURCE_M0 * std::exp(-((t - SOURCE_t0) * (t - SOURCE_t0)) / (2.0 * SIGMA * SIGMA));
}


void applications::exahype2::acoustics::AcousticWaveSolver::initPointSourceLocations(double sourceLocations[NumberOfPointSources][Dimensions]) {
  sourceLocations[0][0] = SOURCE_LOCATION(0);
  sourceLocations[0][1] = SOURCE_LOCATION(1);
  sourceLocations[0][2] = SOURCE_LOCATION(2);
}
#endif


::exahype2::RefinementCommand applications::exahype2::acoustics::AcousticWaveSolver::refinementCriterion(
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
      const double pressure = Q[Vars::p];
      if (pressure >= 0.05) {
        result = ::exahype2::RefinementCommand::Refine;
      } else if (pressure < 0.05) {
        result = ::exahype2::RefinementCommand::Erase;
      }
    }
  }

  return result;
}
