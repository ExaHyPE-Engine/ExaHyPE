// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Landslide.h"

tarch::logging::Log applications::exahype2::landslide::Landslide::_log(
  "applications::exahype2::landslide::Landslide"
);

using s = applications::exahype2::landslide::VariableShortcuts;

void applications::exahype2::landslide::Landslide::initialCondition(
  [[maybe_unused]] double* __restrict__                         Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] bool                                         gridIsConstructed
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  const double elevation = topologyParser.sampleElevation(
    x(0),
    x(1)
  );
  const double initialVolume = topologyParser.sampleInitV(
    x(0),
    x(1)
  );

  Q[s::h] = initialVolume;
  Q[s::z] = elevation;
}


void applications::exahype2::landslide::Landslide::boundaryConditions(
  [[maybe_unused]] const double* __restrict__                   Qinside,
  [[maybe_unused]] double* __restrict__                         Qoutside,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] int                                          normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }

  // Reflecting boundary conditions
  Qoutside[s::hu] = -Qinside[s::hu];
  Qoutside[s::hv] = -Qinside[s::hv];
}


::exahype2::RefinementCommand applications::exahype2::landslide::Landslide::
  refinementCriterion(
    [[maybe_unused]] const double* __restrict__                   Q,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
    [[maybe_unused]] double                                       t
  ) {
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep, h[0]);
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep, h[1]);

  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep, h[0]);
  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep, h[1]);

  auto result = ::exahype2::RefinementCommand::Keep;

  return result;
}
