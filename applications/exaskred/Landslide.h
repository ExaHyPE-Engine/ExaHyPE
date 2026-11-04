// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::landslide {
  /**
   * Material and coupling constants of the landslide layer, built from the generated constants of a
   * scenario.
   */
  struct Physics {
    double gravity;
    double dryHeightThreshold;
    double coulombFriction;
    double inverseTurbulentFrictionLength;
    double slideDensity;
    double waterDensity;
    double waterLevel;
    double waterFrictionDragCoefficient;
    double waterPressureDragCoefficient;
  };

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void sanitiseDryState(
    const T* const NOALIAS QIn,
    T* const NOALIAS       QOut,
    const Physics&         physics
  );

  INLINE_METHOD GPU_CALLABLE_METHOD double getSubmergedGravity(const Physics& physics);

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isSubmergedByBathymetry(const T* const NOALIAS Q, const Physics& physics);

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double getEffectiveGravity(const T* const NOALIAS Q, const Physics& physics);

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double getInterfaceEffectiveGravity(
    const T* const NOALIAS QL,
    const T* const NOALIAS QR,
    const Physics&         physics
  );

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double getWaterFrictionDragSourceSpeed(
    const T* const NOALIAS Q,
    const Physics&         physics,
    const bool             submerged
  );

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double getWaterFrictionDragSourceSpeed(
    const T* const NOALIAS Q,
    const Physics&         physics
  );

  template <class Shortcuts, int NumberOfUnknowns, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const Physics&                               physics,
    const double                                 gravity,
    T* const NOALIAS                             F
  );

  template <class Shortcuts, int NumberOfUnknowns, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const Physics&                               physics,
    T* const NOALIAS                             F
  );

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double getFluxEigenvalue(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const Physics&                               physics,
    const double                                 gravity
  );

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double getFluxEigenvalue(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const Physics&                               physics
  );

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double getEffectiveMaxEigenvalue(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const Physics&                               physics,
    const double                                 gravity,
    const bool                                   submerged,
    const double                                 sourceEigenvalueScaling
  );

  template <class Shortcuts, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD double getEffectiveMaxEigenvalue(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const Physics&                               physics,
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
    const Physics&                               physics,
    const double                                 gravity,
    T* const NOALIAS                             BTimesDeltaQ
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
    const Physics&                               physics,
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
    const Physics&                               physics,
    const double                                 gravity,
    const bool                                   submerged,
    T* const NOALIAS                             S
  );

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
    const T* const NOALIAS                       Q,
    const T* const NOALIAS                       deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const Physics&                               physics,
    T* const NOALIAS                             S
  );
} // namespace applications::exahype2::exaskred::landslide

#include "Landslide.cpph"
