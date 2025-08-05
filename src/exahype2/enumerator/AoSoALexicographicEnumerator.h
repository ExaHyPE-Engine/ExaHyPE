// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::enumerator {
  /**
   * AoSoA Lexicographic Enumerator
   *
   * Enumerates the degrees of freedom assuming that they are
   *
   * - organised as structure of arrays, i.e., if you store quantities (a,b,c) per
   *   degree of freedom (dof), then the array first holds the a values, then all the b
   *   values, then all the c values.
   * - organised lexicographically, i.e. we first enumerate all the degrees of
   *   freedom along the x-axis, then along the y-axis, then along the z-axis.
   * - organised from left to right, bottom-up and front to back.
   * - organised per patch with this pattern, i.e., we first enumerate all the a values
   *   of the first patch, then all the b values, the all the c values. After that
   *   we continue with the data of the second patch.
   *
   * This enumerator enumerates the patches one after the other (AoS). Within each
   * patch, it then however switches to an SoA data layout. The enumeration thus is
   * a hybrid between AoS and SoA. The last property in the enumeration above is the
   * decisive difference to the SoALexicographicEnumerator.
   *
   *
   * @see enumerator.h for a generic description of attribute semantics.
   */
  struct AoSoALexicographicEnumerator {
    inline GPU_CALLABLE_METHOD AoSoALexicographicEnumerator(
      int numberOfCells,
      int numberOfDoFsPerAxisInCell,
      int haloSize,
      int numberOfUnknowns,
      int numberOfAuxiliaryVariables
    ):
      _numberOfCells(numberOfCells),
      _numberOfDoFsPerAxisInCell(numberOfDoFsPerAxisInCell),
      _haloSize(haloSize),
      _numberOfUnknowns(numberOfUnknowns),
      _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables) {}

    /**
     * Access an index
     *
     * The index always refers to the interior of the cells. So you can use negative
     * indices if you want.
     *
     */
    inline GPU_CALLABLE_METHOD int operator()(int cellIndex, const tarch::la::Vector<DIMENSIONS, int>& dofIndex, int unknown) const {
#if DIMENSIONS == 2
      return (cellIndex % _numberOfCells)
        * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfUnknowns + _numberOfAuxiliaryVariables)
        + unknown * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
        + (dofIndex(1) + _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
        + (dofIndex(0) + _haloSize);
#else
      return (cellIndex % _numberOfCells)
        * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfUnknowns + _numberOfAuxiliaryVariables)
        + unknown * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
        + (dofIndex(2) + _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
        + (dofIndex(1) + _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
        + (dofIndex(0) + _haloSize);
#endif
    }

#if !defined(WITH_GPU)
    std::string toString() const;
#endif

    /**
     * Returns the total size.
     */
    inline GPU_CALLABLE_METHOD int size() const {
#if DIMENSIONS == 2
      return _numberOfCells
        * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
        * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
        * (_numberOfUnknowns + _numberOfAuxiliaryVariables);
#else
      return _numberOfCells
        * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
        * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
        * (_numberOfDoFsPerAxisInCell + 2 * _haloSize)
        * (_numberOfUnknowns + _numberOfAuxiliaryVariables);
#endif
    }

    inline GPU_CALLABLE_METHOD int numberOfDofsPerCell() const {
#if DIMENSIONS == 2
      return (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize);
#else
      return (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize);
#endif
    }

    const int _numberOfCells;
    const int _numberOfDoFsPerAxisInCell;
    const int _haloSize;
    const int _numberOfUnknowns;
    const int _numberOfAuxiliaryVariables;
  };
} // namespace exahype2::enumerator
