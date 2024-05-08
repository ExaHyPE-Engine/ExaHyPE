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
   * Default enumerator for all DoFs in ExaHyPE 2
   *
   * This enumerator is meant for faces. You don't need this if you work purely
   * within a cell or a patch, but you need an enumeration of the faces if you
   * work patch-wisely.
   *
   * ## Enumeration
   *
   * The DoFs along a face are enumerated lexicographically.
   *
   * So if we talk about faceNumber 0, we have an enumeration of
   *
   *               6 | 7
   *               4 | 5
   *               2 | 3
   *               0 | 1
   *
   * If we talk about number 1, we have
   *
   *               7  8  9 10 11 12 13
   *               -------------------
   *               0  1  2  3  4  5  6
   *
   * The ASCII art above gives only examples for a halo size of one, where we
   * have two cells along the normal. The extension to bigger halo sizes is
   * straightforward:
   *
   *
   *               21 22 23 24 25 26 27
   *               14 15 16 17 18 19 20
   *               --------------------
   *                7  8  9 10 11 12 13
   *                0  1  2  3  4  5  6
   *
   *
   * ## Differences to volumetric enumerators
   *
   * Face enumerators always refer only to one batch, i.e., one piece of face
   * data. ExaHyPE's volumetric enumerators in contrast can index a whole
   * batch of cells in one go.
   *
   */
  struct FaceAoSLexicographicEnumerator {
    /**
     *
     * @param faceNumber An integer value between 0 (inclusive) and
     *   2*Dimensions (exklusive). Standard Peano enumeration of faces.
     *   While you can pass in a number bigger than Dimensions, i.e.,
     *   while you can distinguish between left and right face, both
     *   the left and right face have the same dof enumeration. Consequently,
     *   the initialisation with i gives you the same as the initialisation
     *   with i+Dimensions.
     *
     * @param numberOfDoFsPerAxisInCell Number of volumes per direction
     *   if you work with block-structured Finite Volumes, order+1 if you
     *   work in a DG regime.
     *
     * @param haloSize For a Finite Volume solver, this is one if each
     *   patch is augmented with one halo volume around it. For a DG solver,
     *   it is 1 if you project only one quantity, i.e., the solution. However,
     *   you might have a value bigger than 1 if you also project higher order
     *   properties such as the first derivative along the normal. Some
     *   code snippets within ExaHyPE 2 call the haloSize overlap.
     */
    constexpr GPUCallableInlineMethod FaceAoSLexicographicEnumerator(int faceNumber, int numberOfDoFsPerAxisInCell, int haloSize, int unknowns, int numberOfAuxiliaryVariables):
      _faceNumber(faceNumber),
      _numberOfDoFsPerAxisInCell(numberOfDoFsPerAxisInCell),
      _haloSize(haloSize),
      _unknowns(unknowns),
      _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables) {}

    /**
     * Access a dof
     */
    GPUCallableInlineMethod int operator()(const tarch::la::Vector<Dimensions, int>& dofIndex, int unknown) const {
      int base   = 1;
      int result = 0;
      for (int d = 0; d < Dimensions; d++) {
        result += dofIndex(d) * base;
        if (d == _faceNumber % Dimensions) {
#if defined(GPUOffloadingOff)
          assertion2(dofIndex(d) >= 0, toString(), dofIndex);
          assertion2(dofIndex(d) < 2 * _haloSize, toString(), dofIndex);
#endif
          base *= (2 * _haloSize);
        } else {
#if defined(GPUOffloadingOff)
          assertion2(dofIndex(d) >= 0, toString(), dofIndex);
          assertion2(dofIndex(d) < _numberOfDoFsPerAxisInCell, toString(), dofIndex);
#endif
          base *= _numberOfDoFsPerAxisInCell;
        }
      }
      return result * (_unknowns + _numberOfAuxiliaryVariables) + unknown;
    }

#if defined(GPUOffloadingOff)
    std::string toString() const;
#endif

    GPUCallableInlineMethod int size() const {
      if constexpr (Dimensions == 2) {
        return (_numberOfDoFsPerAxisInCell) * (2 * _haloSize) * (_unknowns + _numberOfAuxiliaryVariables);
      } else {
        return (_numberOfDoFsPerAxisInCell * _numberOfDoFsPerAxisInCell) * (2 * _haloSize) * (_unknowns + _numberOfAuxiliaryVariables);
      }
    }

    const int _faceNumber;
    const int _numberOfDoFsPerAxisInCell;
    const int _haloSize;
    const int _unknowns;
    const int _numberOfAuxiliaryVariables;
  };
} // namespace exahype2::enumerator
