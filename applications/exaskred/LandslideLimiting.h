// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "applications/exaskred/Landslide.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::landslide {
  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isAdmissibleGranularLayerState(
    const T* const NOALIAS Q,
    const Physics&         physics
  );

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isAdmissibleGranularLayerCell(
    const T* const NOALIAS                       Q,
    const int                                    numberOfStates,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const Physics&                               physics,
    const double                                 maxFrictionToWaveSpeedRatio
  );

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD bool hasAdmissibleGranularLayerHeightCurvature2D(
    const T* const NOALIAS Q,
    const int              nodesPerAxis,
    const Physics&         physics,
    const double           maxCurvatureRatio
  );

  template <class Shortcuts, typename ObservableT, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void mapGranularLayerDmpObservables(
    ObservableT* const NOALIAS observables,
    const T* const NOALIAS     Q
  );

  INLINE_METHOD GPU_CALLABLE_METHOD double granularLayerDmpRelaxationParameter(
    const int      observable,
    const double   specifiedRelaxationParameter,
    const Physics& physics
  );
} // namespace applications::exahype2::exaskred::landslide

#include "LandslideLimiting.cpph"
