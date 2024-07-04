// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SWE.h"

::tarch::logging::Log applications::exahype2::swe::SWE::_log(
  "applications::exahype2::swe::SWE"
);

using s = applications::exahype2::swe::VariableShortcuts;

void applications::exahype2::swe::SWE::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  bool                                         gridIsConstructed
) {
  const double bathymetryBeforeEarthquake = topologyParser.sampleBathymetry(
    x(0),
    x(1)
  );
  const double displacement = topologyParser.sampleDisplacement(x(0), x(1));
  const double bathymetryAfterEarthquake = bathymetryBeforeEarthquake
                                           + displacement;

  Q[s::h]      = -std::min(bathymetryBeforeEarthquake, 0.0);
  Q[s::hu + 0] = 0.0;
  Q[s::hu + 1] = 0.0;
  Q[s::b]      = bathymetryAfterEarthquake;
}


void applications::exahype2::swe::SWE::boundaryConditions(
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


::exahype2::RefinementCommand applications::exahype2::swe::SWE::
  refinementCriterion(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t
  ) {
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep, h[0]);
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep, h[1]);

  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep, h[0]);
  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep, h[1]);

  auto result = ::exahype2::RefinementCommand::Keep;

  if (x(0) >= 6e6 and x(1) <= 2e6) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
}
