// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <functional>
#include <string>

#include "peano4/utils/Globals.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

namespace exahype2::enumerator {
  /**
   * Default enumerator for dofs
   *
   * We assume that we have a lexicographic ordering of the cell data and
   * all data therein are ordered lexicographically and stored in as AoS.
   *
   */
  struct SingleDoFEnumerator {
    constexpr GPUCallableInlineMethod SingleDoFEnumerator(int unknowns, int numberOfAuxiliaryVariables):
      _unknowns(unknowns),
      _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables) {}

    /**
     * Access an index
     *
     * The index always refers to the interior of the cells. So you can use negative
     * indices if you want.
     *
     */
    GPUCallableInlineMethod int operator()(int, const tarch::la::Vector<Dimensions, int>&, int unknown) const InlineMethod { return unknown; }

#if defined(GPUOffloadingOff)
    std::string toString() const;
#endif

    GPUCallableInlineMethod int size() const { return (_unknowns + _numberOfAuxiliaryVariables); }

    const int _unknowns;
    const int _numberOfAuxiliaryVariables;
  };
} // namespace exahype2::enumerator
