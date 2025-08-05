// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::enumerator {
  /**
   * Array of struct enumerator
   *
   * This is the default enumerator for all DoFs in ExaHyPE 2.
   *
   * We assume that we have a lexicographic ordering of the cell data or
   * DG degree of freedom (quadrature points) and all data therein are
   * ordered lexicographically and stored in as AoS: All data are
   *
   * - organised as array of structs, i.e. if you store quantities (a,b,c) per
   *   degree of freedom (dof), then the array first holds the a value of the first
   *   dof, then the b value, then the c value, and then it continues with the
   *   a value of the next dof.
   * - organised lexicographically, i.e. we first enumerate all the degrees of
   *   freedom along the x-axis, then along the y-axis, then along the z-axis.
   * - organised from left to right, bottom-up and front to back.
   * - organised one cell after another, i.e. we first enumerate all the values
   *   of the first cell, before we continue with any value of the second
   *   cell.
   *
   * @see enumerator.h for a generic description of attribute semantics.
   *
   *
   * @param numberOfCells The cells are enumerated one after another, i.e.
   *   we work with an array of cells hosting arrays of structs.
   *
   * @param haloSize Number of dofs around the cell which do not carry
   *   active information, i.e. data you write to.
   */
  struct AoSLexicographicEnumerator {
    inline GPU_CALLABLE_METHOD AoSLexicographicEnumerator(
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
      return (cellIndex % _numberOfCells) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfUnknowns + _numberOfAuxiliaryVariables)
           + (dofIndex(1) + _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfUnknowns + _numberOfAuxiliaryVariables)
           + (dofIndex(0) + _haloSize) * (_numberOfUnknowns + _numberOfAuxiliaryVariables)
           + unknown;
#else
      return (cellIndex % _numberOfCells) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfUnknowns + _numberOfAuxiliaryVariables)
           + (dofIndex(2) + _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfUnknowns + _numberOfAuxiliaryVariables)
           + (dofIndex(1) + _haloSize) * (_numberOfDoFsPerAxisInCell + 2 * _haloSize) * (_numberOfUnknowns + _numberOfAuxiliaryVariables)
           + (dofIndex(0) + _haloSize) * (_numberOfUnknowns + _numberOfAuxiliaryVariables)
           + unknown;
#endif
    }

#if !defined(WITH_GPU)
    std::string toString() const;
#endif

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
