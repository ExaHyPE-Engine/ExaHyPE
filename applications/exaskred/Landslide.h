// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::landslide {
  struct Physics;

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD void sanitiseDryState(const State& QIn, State& QOut, const Physics& physics);

  INLINE_METHOD GPU_CALLABLE_METHOD double getSubmergedGravity(const Physics& physics);

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isSubmergedByBathymetry(const State& Q, const Physics& physics);

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double getEffectiveGravity(const State& Q, const Physics& physics);

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double getInterfaceEffectiveGravity(
    const State&   QL,
    const State&   QR,
    const Physics& physics
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double getWaterFrictionDragSourceSpeed(
    const State&   Q,
    const Physics& physics,
    bool           submerged
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double getWaterFrictionDragSourceSpeed(const State& Q, const Physics& physics);

  template <typename State, typename Flux>
  INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const State&                                 Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const Physics&                               physics,
    double                                       gravity,
    Flux&                                        F
  );

  template <typename State, typename Flux>
  INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const State&                                 Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const Physics&                               physics,
    Flux&                                        F
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double getFluxEigenvalue(
    const State&                                 Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const Physics&                               physics,
    double                                       gravity
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double getFluxEigenvalue(
    const State&                                 Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const Physics&                               physics
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double getEffectiveMaxEigenvalue(
    const State&                                 Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const Physics&                               physics,
    double                                       gravity,
    bool                                         submerged,
    double                                       sourceEigenvalueScaling
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD double getEffectiveMaxEigenvalue(
    const State&                                 Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    const Physics&                               physics,
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
    const Physics&                               physics,
    double                                       gravity,
    NonconservativeProduct&                      BTimesDeltaQ
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
    const Physics&                               physics,
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
    const Physics&                               physics,
    double                                       gravity,
    bool                                         submerged,
    Source&                                      S
  );

  template <typename State, typename Gradient, typename Source>
  INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
    const State&                                 Q,
    const Gradient&                              deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    const Physics&                               physics,
    Source&                                      S
  );
} // namespace applications::exahype2::exaskred::landslide

/**
 * Physics stores the material and coupling constants of the landslide layer. A scenario copies them
 * from its generated constants.
 */
struct applications::exahype2::exaskred::landslide::Physics final {
  double gravity{};
  double dryHeightThreshold{};
  double coulombFriction{};
  double inverseTurbulentFrictionLength{};
  double slideDensity{};
  double waterDensity{};
  double waterLevel{};
  double waterFrictionDragCoefficient{};
  double waterPressureDragCoefficient{};
};

#include "Landslide.cpph"
