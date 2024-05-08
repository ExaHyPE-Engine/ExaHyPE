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
   * SoA Lexicographic Enumerator
   *
   * Enumerates the degrees of freedom assuming that they are
   *
   * - organised as structure of arrays, i.e., if you store quantities (a,b,c) per
   *   degree of freedom, then the array first holds the a values, then all the b
   *   values, then all the c values.
   * - organised lexicographically, i.e., we first enumerate all the degrees of
   *   freedom along the x-axis, then along the y-axis, then along the z-axis.
   * - organised from left to right, bottom-up and front to back.
   * - organised one cell after another, i.e., we first enumerate all the a values
   *   of the first cell, followed by all the a unknowns of the second cell, and
   *   so forth. After the a values of the last cell, we continue with the b values
   *   in the examples with (a,b,c) per degree of freedom.
   *
   *
   * This native SoA enumerator uses SoA within the cells plus over cells: It
   * first enumerates all Q[0] data from cell one, then all Q[0] from cell two,
   * then all Q[0] from cell three, and so forth. After that, it continues with
   * Q[1]. This means: The enumerator interleaves the individual cells.
   *
   * @see enumerator.h for a generic description of attribute semantics.
   */
  struct SoALexicographicEnumerator {
    constexpr GPUCallableInlineMethod SoALexicographicEnumerator(int numberOfCells, int numberOfDoFsPerAxisInCell, int haloSize, int unknowns, int numberOfAuxiliaryVariables):
      _numberOfCells(numberOfCells),
      _numberOfDoFsPerAxisInCell(numberOfDoFsPerAxisInCell),
      _haloSize(haloSize),
      _unknowns(unknowns),
      _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables) {}

    /**
     * Access an index
     *
     * The index always refers to the interior of the cells. So you can use negative
     * indices if you want.
     */
    GPUCallableInlineMethod int operator()(int cellIndex, const tarch::la::Vector<Dimensions, int>& volumeIndex, int unknown) const InlineMethod {
      if constexpr (Dimensions == 2) {
        return unknown * _numberOfCells * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
               + (cellIndex % _numberOfCells) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
               + (volumeIndex(1) + _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) + (volumeIndex(0) + _haloSize);
      } else {
        return unknown * _numberOfCells * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
               + (cellIndex % _numberOfCells) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
                   * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
               + (volumeIndex(2) + _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
               + (volumeIndex(1) + _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) + (volumeIndex(0) + _haloSize);
      }
    }

#if defined(GPUOffloadingOff)
    std::string toString() const;
#endif

    /**
     * Returns the total size.
     */
    GPUCallableInlineMethod int size() const InlineMethod {
      if constexpr (Dimensions == 2) {
        return _numberOfCells * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_unknowns + _numberOfAuxiliaryVariables);
      } else {
        return _numberOfCells * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
               * (_unknowns + _numberOfAuxiliaryVariables);
      }
    }

    const int _numberOfCells;
    const int _numberOfDoFsPerAxisInCell;
    const int _haloSize;
    const int _unknowns;
    const int _numberOfAuxiliaryVariables;
  };
} // namespace exahype2::enumerator
