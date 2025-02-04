// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::enumerator {
  /**
   * We assume that we have a lexicographic ordering of the cell data and
   * all data therein are ordered lexicographically and stored in as AoS.
   */
  struct SingleDoFEnumerator {
    inline GPUCallableMethod SingleDoFEnumerator(
      int numberOfUnknowns,
      int numberOfAuxiliaryVariables
    ) InlineMethod:
      _numberOfUnknowns(numberOfUnknowns),
      _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables) {}

    /**
     * Access an index
     *
     * The index always refers to the interior of the cells. So you can use negative
     * indices if you want.
     *
     */
    inline GPUCallableMethod int operator()(int, const tarch::la::Vector<Dimensions, int>&, int unknown) const InlineMethod {
      return unknown;
    }

#if defined(GPUOffloadingOff)
    std::string toString() const;
#endif

    inline GPUCallableMethod int size() const InlineMethod {
      return (_numberOfUnknowns + _numberOfAuxiliaryVariables);
    }

    const int _numberOfUnknowns;
    const int _numberOfAuxiliaryVariables;
  };
} // namespace exahype2::enumerator
