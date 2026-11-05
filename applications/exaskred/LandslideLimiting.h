// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "applications/exaskred/Landslide.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::landslide {
  template <auto Height, auto Momentum, typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isAdmissibleGranularLayerState(const State& Q, const Physics& physics);

  template <auto Height, auto Momentum, typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isAdmissibleGranularLayerCell(
    std::span<const State>                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const Physics&                               physics,
    double                                       maxFrictionToWaveSpeedRatio
  );

  template <auto Height, typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD bool hasAdmissibleGranularLayerHeightCurvature2D(
    std::span<const State> Q,
    int                    nodesPerAxis,
    const Physics&         physics,
    double                 maxCurvatureRatio
  );

  template <typename State, typename Observables>
  INLINE_METHOD GPU_CALLABLE_METHOD void mapGranularLayerDmpObservables(Observables& observables, const State& Q);

  INLINE_METHOD GPU_CALLABLE_METHOD double getGranularLayerDMPRelaxationParameter(
    int            observable,
    double         specifiedRelaxationParameter,
    const Physics& physics
  );
} // namespace applications::exahype2::exaskred::landslide

#include "LandslideLimiting.cpph"
