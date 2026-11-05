// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::shallowwater {
  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD T regularisedVelocity(T waterHeight, T momentum, double minimumWaterHeight);

  template <typename State, typename Flux>
  INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const State&                                 Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
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
    double                                       gravity,
    double                                       minimumWaterHeight
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
    double                                       gravity,
    double                                       minimumWaterHeight,
    NonconservativeProduct&                      BTimesDeltaQ
  );
} // namespace applications::exahype2::exaskred::shallowwater

#include "ShallowWater.cpph"
