// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::landslide {
  template <class T, class Shortcuts>
  static inline GPU_CALLABLE_METHOD void flux(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    T* const NOALIAS                             F
  );

  template <class T, class Shortcuts>
  static inline GPU_CALLABLE_METHOD double maxEigenvalue(
    const T* const NOALIAS                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal
  );

  template <class T, class Shortcuts>
  static inline GPU_CALLABLE_METHOD void nonconservativeProduct(
    const T* const NOALIAS                       Q,
    const T* const NOALIAS                       deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    T* const NOALIAS                             BTimesDeltaQ
  );

  template <class T, class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  static inline GPU_CALLABLE_METHOD void diffusiveSourceTerm(
    const T* const NOALIAS                       Q,
    const T* const NOALIAS                       deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    T* const NOALIAS                             S
  );

} // namespace applications::exahype2::landslide

#include "PDE.cpph"

namespace applications::exahype2::ShallowWater {
  constexpr double g          = landslide::g;
  constexpr double hThreshold = landslide::hThreshold;
} // namespace applications::exahype2::ShallowWater
