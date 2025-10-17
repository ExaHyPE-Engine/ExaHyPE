// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::ShallowWater {
  namespace FrictionLaws {
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

    template <class T, class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
    static inline GPU_CALLABLE_METHOD void sourceTermADERDG(
      const T* const NOALIAS                       Q,
      const T* const NOALIAS                       deltaQ,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double                                 t,
      const double                                 dt,
      T* const NOALIAS                             S
    );

    template <class T, class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
    static inline GPU_CALLABLE_METHOD void sourceTermFV(
      const T* const NOALIAS                       Q,
      const T* const NOALIAS                       deltaQ,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double                                 t,
      const double                                 dt,
      T* const NOALIAS                             S
    );
  } // namespace FrictionLaws
} // namespace applications::exahype2::ShallowWater

#include "FrictionLaws.cpph"
