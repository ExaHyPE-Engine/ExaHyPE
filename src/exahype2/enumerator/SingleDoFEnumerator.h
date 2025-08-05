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
    inline GPU_CALLABLE_METHOD SingleDoFEnumerator(
      int numberOfUnknowns,
      int numberOfAuxiliaryVariables
    ):
      _numberOfUnknowns(numberOfUnknowns),
      _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables) {}

    /**
     * Access an index
     *
     * The index always refers to the interior of the cells. So you can use negative
     * indices if you want.
     *
     */
    inline GPU_CALLABLE_METHOD int operator()(int, const tarch::la::Vector<DIMENSIONS, int>&, int unknown) const {
      return unknown;
    }

#if !defined(WITH_GPU)
    std::string toString() const;
#endif

    inline GPU_CALLABLE_METHOD int size() const {
      return (_numberOfUnknowns + _numberOfAuxiliaryVariables);
    }

    const int _numberOfUnknowns;
    const int _numberOfAuxiliaryVariables;
  };
} // namespace exahype2::enumerator
