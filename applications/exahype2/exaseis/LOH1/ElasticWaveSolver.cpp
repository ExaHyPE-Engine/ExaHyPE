// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ElasticWaveSolver.h"

tarch::logging::Log applications::exahype2::exaseis::ElasticWaveSolver::_log("applications::exahype2::exaseis::ElasticWaveSolver");

using Vars = applications::exahype2::exaseis::VariableShortcuts;


void applications::exahype2::exaseis::ElasticWaveSolver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    index,
  bool                                         gridIsConstructed
) {
  for (int i = 0; i < NumberOfUnknowns; i++) {
    Q[i] = 0.0;
  }

  // Isotropic
  Q[Vars::rho] = x[1] < 1.0 ? 2.6 : RHO;
  Q[Vars::cp]  = x[1] < 1.0 ? 4.0 : P_WAVE_SPEED;
  Q[Vars::cs]  = x[1] < 1.0 ? 2.0 : S_WAVE_SPEED;
}


void applications::exahype2::exaseis::ElasticWaveSolver::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  int                                          normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


::exahype2::RefinementCommand applications::exahype2::exaseis::ElasticWaveSolver::refinementCriterion(
  const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t
) {
  auto result = ::exahype2::RefinementCommand::Keep;

  if (tarch::la::equals(t, 0.0)) {
    tarch::la::Vector<Dimensions, double> sourceLocation({SOURCE_LOCATION_X, SOURCE_LOCATION_Y, SOURCE_LOCATION_Z});
    if (tarch::la::norm2(x - sourceLocation) < SOURCE_RADIUS) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  return result;
}


#ifdef USE_POINT_SOURCE
void applications::exahype2::exaseis::ElasticWaveSolver::initPointSourceLocations(double sourceLocations[NumberOfPointSources][Dimensions]) {
  sourceLocations[0][0] = SOURCE_LOCATION_X;
  sourceLocations[0][1] = SOURCE_LOCATION_Y;
  sourceLocations[0][2] = SOURCE_LOCATION_Z;
}
#endif
