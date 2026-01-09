// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::ShallowWater {
  template <class T, class Shortcuts, int NumberOfUnknowns>
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

  template <class T, class Shortcuts, int NumberOfUnknowns>
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
} // namespace applications::exahype2::ShallowWater

#include "PDE.cpph"
