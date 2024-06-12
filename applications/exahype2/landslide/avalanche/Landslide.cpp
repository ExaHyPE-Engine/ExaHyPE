// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Landslide.h"

::tarch::logging::Log applications::exahype2::landslide::Landslide::_log(
  "applications::exahype2::landslide::Landslide"
);

using s = applications::exahype2::landslide::VariableShortcuts;

void applications::exahype2::landslide::Landslide::initialCondition(
  double* __restrict__ Q,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  bool                                           gridIsConstructed
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
  const tarch::la::Vector<Dimensions, double>& granularMaterialCenter = {
    {0.15, 0.35}};
  const bool nearCentre = tarch::la::norm2(x - granularMaterialCenter) < 0.10;

  Q[s::h] = (nearCentre ? 0.008 : 0.0); // Granular material height (h)
}


void applications::exahype2::landslide::Landslide::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  int                                            normal
) {
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }

  // Reflecting boundary conditions
  Qoutside[s::hu + 0] = -Qinside[s::hu + 0];
  Qoutside[s::hu + 1] = -Qinside[s::hu + 1];
}


::exahype2::RefinementCommand applications::exahype2::landslide::Landslide::
  refinementCriterion(
    const double* __restrict__ Q,
    const ::tarch::la::Vector<Dimensions, double>& x,
    const ::tarch::la::Vector<Dimensions, double>& h,
    double                                         t
  ) {
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep, h[0]);
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep, h[1]);

  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep, h[0]);
  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep, h[1]);

  auto result = ::exahype2::RefinementCommand::Keep;

  if (x(0) < 0.79) {
    result = ::exahype2::RefinementCommand::Refine;
  } else {
    result = ::exahype2::RefinementCommand::Erase;
  }

  return result;
}
