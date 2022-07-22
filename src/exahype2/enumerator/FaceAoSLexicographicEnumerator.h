// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ENUMERATOR_FACE_AOS_LEXICOGRAPHIC_ENUMERATOR_H_
#define _EXAHYPE2_ENUMERATOR_FACE_AOS_LEXICOGRAPHIC_ENUMERATOR_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>


namespace exahype2 {
  namespace enumerator {
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
     * Face enumerators always refer only to one batch, i.e. one piece of face
     * data. ExaHyPE's volumetric enumerators in contrast can index a whole
     * batch of cells in one go.
     *
     *
     */
    struct FaceAoSLexicographicEnumerator {
      public:
        /**
         *
         * @param faceNumber An integer value between 0 (inclusive) and
         *   2*Dimensions (exklusive). Standard Peano enumeration of faces.
         *
         * @param numberOfDoFsPerAxisInPatch Number of volumes per direction
         *   if you work with block-structured Finite Volumes. order+1 if you
         *   work in a DG regime.
         *
         * @param haloSize For a Finite Volume solver, this is one if each
         *   patch is augmented with one halo volume around it. For a DG solver,
         *   it is 1 if you project only one quantity, i.e. the solution. However,
         *   you might have a value bigger than 1 if you also project higher order
         *   properties such as the first derivative along the normal. Some
         *   code snippets within ExaHyPE call the haloSize overlap.
         */
        FaceAoSLexicographicEnumerator(
          int faceNumber,
          int numberOfDoFsPerAxisInPatch,
          int haloSize,
          int unknowns,
          int numberOfAuxiliaryVariables
        ):
          _faceNumber(faceNumber),
          _numberOfDoFsPerAxisInPatch(numberOfDoFsPerAxisInPatch),
          _haloSize(haloSize),
          _unknowns(unknowns),
          _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables){}

        /**
         * Access a dof
         */
        int operator()(
          const tarch::la::Vector<Dimensions,int>&  dofIndex,
          int unknown
        ) const;

        #if !defined(GPUOffloadingOMP)
        std::string toString() const;
        #endif

        int size() const;

        const int _faceNumber;
        const int _numberOfDoFsPerAxisInPatch;
        const int _haloSize;
        const int _unknowns;
        const int _numberOfAuxiliaryVariables;
    };
  }
}


#endif

