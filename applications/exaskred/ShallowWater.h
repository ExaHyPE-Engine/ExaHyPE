// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::shallowwater {
  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD T
  regularisedVelocity(const T waterHeight, const T momentum, const double minimumWaterHeight);

  template <class Shortcuts, int NumberOfUnknowns, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const double                                 minimumWaterHeight,
    T* const NOALIAS                             F
  );

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD T maxEigenvalue(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const double                                 gravity,
    const double                                 minimumWaterHeight
  );

  template <class Shortcuts, int NumberOfUnknowns, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
    const T* const NOALIAS                       Q,
    const T* const NOALIAS                       deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const double                                 gravity,
    const double                                 minimumWaterHeight,
    T* const NOALIAS                             BTimesDeltaQ
  );
} // namespace applications::exahype2::exaskred::shallowwater

#include "ShallowWater.cpph"
