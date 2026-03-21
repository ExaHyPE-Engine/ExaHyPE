// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::ShallowWater {
  namespace meteotsunami {
    template <class T, class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
    static inline GPU_CALLABLE_METHOD void sourceTerm(
      const T* const NOALIAS                       Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double                                 t,
      const double                                 dt,
      T* const NOALIAS                             S
    );
  } // namespace meteotsunami
} // namespace applications::exahype2::ShallowWater

#include "SourceTerm.cpph"
