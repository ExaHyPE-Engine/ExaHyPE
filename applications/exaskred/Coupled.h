// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "applications/exaskred/AugmentedState.h"
#include "applications/exaskred/Landslide.h"
#include "applications/exaskred/ShallowWater.h"
#include "applications/exaskred/WellBalancedRusanov.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::coupled {
  template <typename T>
  struct WaterLayer;

  template <typename T>
  struct SlideLayer;

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isSlideSubmerged(const State& Q, double minimumWaterHeight);

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double getSlideGravity(
    const State&              Q,
    const landslide::Physics& physics,
    double                    minimumWaterHeight
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD void sanitiseDryLayers(
    const State&              QIn,
    State&                    QOut,
    const landslide::Physics& physics,
    double                    minimumWaterHeight
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD void addRemovedLayersToBed(
    const State&                    Q,
    const State&                    sanitisedQ,
    WaterLayer<decltype(State::h)>& waterLayer
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD WaterLayer<decltype(State::h)> getWaterLayerOf(const State& Q);

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD SlideLayer<decltype(State::h)> getSlideLayerOf(const State& Q);

  template <typename State, typename Flux>
  INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const State&                                 Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const landslide::Physics&                    physics,
    double                                       minimumWaterHeight,
    Flux&                                        F
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
    const State&                                 Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const landslide::Physics&                    physics,
    double                                       minimumWaterHeight,
    double                                       sourceEigenvalueScaling
  );

  template <typename State, typename Difference, typename NonconservativeProduct>
  INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
    const State&                                 Q,
    const Difference&                            deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const landslide::Physics&                    physics,
    double                                       minimumWaterHeight,
    NonconservativeProduct&                      BTimesDeltaQ
  );

  template <typename State, typename Gradient, typename Source>
  INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
    const State&                                 Q,
    const Gradient&                              deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    const landslide::Physics&                    physics,
    double                                       minimumWaterHeight,
    Source&                                      S
  );

  template <typename FaceStates, typename FaceFluxes>
  INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblem(
    const FaceStates&                            QL,
    const FaceStates&                            QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const landslide::Physics&                    physics,
    double                                       minimumWaterHeight,
    FaceFluxes&                                  FL,
    FaceFluxes&                                  FR
  );

  template <typename State, typename Flux>
  INLINE_METHOD GPU_CALLABLE_METHOD double solveSubcellRiemannProblem(
    const State&                                 QL,
    const State&                                 QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const landslide::Physics&                    physics,
    double                                       minimumWaterHeight,
    Flux&                                        FL,
    Flux&                                        FR
  );
} // namespace applications::exahype2::exaskred::coupled

/**
 * WaterLayer stores the thickness h, the momenta hu and the bed b of the water of a coupled state. The
 * bed b is the terrain plus the granular layer.
 */
template <typename T>
struct applications::exahype2::exaskred::coupled::WaterLayer final {
  T                                h{};
  tarch::la::Vector<DIMENSIONS, T> hu{};
  T                                b{};
};

/**
 * SlideLayer stores the thickness h, the momenta hu, the terrain z and the water depth above the
 * granular layer of a coupled state. The water depth decides whether buoyancy reduces the gravity.
 */
template <typename T>
struct applications::exahype2::exaskred::coupled::SlideLayer final {
  T                                h{};
  tarch::la::Vector<DIMENSIONS, T> hu{};
  T                                z{};
  T                                waterDepth{};
};

#include "Coupled.cpph"
