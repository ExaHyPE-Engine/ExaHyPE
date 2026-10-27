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
  /**
   * Maps the indices h, hu and z that the landslide kernels read to the granular layer of the
   * coupled state.
   */
  template <class Shortcuts>
  struct SlideLayer {
    static constexpr int h  = Shortcuts::lsh;
    static constexpr int hu = Shortcuts::lshu;
    static constexpr int z  = Shortcuts::z;
  };

  /**
   * Face view of the water layer: Thickness, momenta and the effective bathymetry, the terrain plus
   * the granular layer.
   */
  struct WaterView {
    static constexpr int h    = 0;
    static constexpr int hu   = 1;
    static constexpr int hv   = 2;
    static constexpr int b    = 3;
    static constexpr int Size = 4;
  };

  /**
   * Face view of the granular layer: Thickness, momenta, the terrain and the depth of the water
   * above the layer. A positive water depth switches on buoyancy.
   */
  struct SlideView {
    static constexpr int h          = 0;
    static constexpr int hu         = 1;
    static constexpr int hv         = 2;
    static constexpr int z          = 3;
    static constexpr int WaterDepth = 4;
    static constexpr int Size       = 5;
  };

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isSlideSubmerged(const T* const NOALIAS Q, const double minimumWaterHeight);

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double getSlideGravity(
    const T* const NOALIAS    Q,
    const landslide::Physics& physics,
    const double              minimumWaterHeight
  );

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void sanitiseDryLayers(
    const T* const NOALIAS    QIn,
    T* const NOALIAS          QOut,
    const landslide::Physics& physics,
    const double              minimumWaterHeight
  );

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void gatherWaterView(const T* const NOALIAS Q, T* const NOALIAS view);

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void gatherSlideView(const T* const NOALIAS Q, T* const NOALIAS view);

  template <class Shortcuts, int NumberOfUnknowns, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const landslide::Physics&                    physics,
    const double                                 minimumWaterHeight,
    T* const NOALIAS                             F
  );

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const landslide::Physics&                    physics,
    const double                                 minimumWaterHeight,
    const double                                 sourceEigenvalueScaling
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
    const landslide::Physics&                    physics,
    const double                                 minimumWaterHeight,
    T* const NOALIAS                             BTimesDeltaQ
  );

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
    const T* const NOALIAS                       Q,
    const T* const NOALIAS                       deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const landslide::Physics&                    physics,
    const double                                 minimumWaterHeight,
    T* const NOALIAS                             S
  );

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, int Order, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblem(
    const T* const NOALIAS                       QL,
    const T* const NOALIAS                       QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const landslide::Physics&                    physics,
    const double                                 minimumWaterHeight,
    T* const NOALIAS                             FL,
    T* const NOALIAS                             FR
  );

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double solveSubcellRiemannProblem(
    const T* const NOALIAS                       QL,
    const T* const NOALIAS                       QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const landslide::Physics&                    physics,
    const double                                 minimumWaterHeight,
    T* const NOALIAS                             FL,
    T* const NOALIAS                             FR
  );
} // namespace applications::exahype2::exaskred::coupled

#include "Coupled.cpph"
