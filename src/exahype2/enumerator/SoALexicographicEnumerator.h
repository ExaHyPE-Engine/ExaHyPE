// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ENUMERATOR_SOA_LEXICOGRAPHIC_ENUMERATOR_H_
#define _EXAHYPE2_ENUMERATOR_SOA_LEXICOGRAPHIC_ENUMERATOR_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>
#include <iostream>


namespace exahype2 {
  namespace enumerator {
  /**
   * This native SoA enumerator uses SoA within the patches plus over patches: It
   * first enumerates all Q[0] data from patch one, then all Q[0] from patch two,
   * then all Q[0] from patch three, and so forth. After that, it continues with
   * Q[1]. This means: The enumerator interleaves the individual patches.
   */
    #if defined(GPUOffloadingOMP)
//      #pragma omp declare target
    #endif
    struct SoALexicographicEnumerator {
      SoALexicographicEnumerator(int numberOfPatches, int numberOfVolumesPerAxisInPatch, int haloSize, int unknowns, int numberOfAuxiliaryVariables):
        _numberOfPatches(numberOfPatches),
        _numberOfVolumesPerAxisInPatch(numberOfVolumesPerAxisInPatch),
        _haloSize(haloSize),
        _unknowns(unknowns),
        _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables){}

      /**
       * Access an index
       *
       * The index always refers to the interior of the patches. So you can use negative
       * indices if you want.
       *
       * @todo modulo Operator muss ich noch dokumentieren
       */
      int operator()(
        int patchIndex,
        const tarch::la::Vector<Dimensions,int>&  volumeIndex,
        int unknown
      ) const InlineMethod {
        if (Dimensions==2) {
          return (patchIndex % _numberOfPatches) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
               + (volumeIndex(1)+_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
               + (volumeIndex(0)+_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
               + unknown;
        }
        else {
          return (patchIndex % _numberOfPatches) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
               + (volumeIndex(2)+_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
               + (volumeIndex(1)+_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
               + (volumeIndex(0)+_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
               + unknown;
        }
      }

      #if !defined(GPUOffloadingOMP)
      std::string toString() const;
#endif


      /**
       * Returns the total size.
       */
      int size() const {
        if (Dimensions==2) {
          return _numberOfPatches
              * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
              * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
              * (_unknowns+_numberOfAuxiliaryVariables);
        }
        else {
          return _numberOfPatches
              * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
              * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
              * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
              * (_unknowns+_numberOfAuxiliaryVariables);
        }
      }


      const int _numberOfPatches;
      const int _numberOfVolumesPerAxisInPatch;
      const int _haloSize;
      const int _unknowns;
      const int _numberOfAuxiliaryVariables;
    };
    #if defined(GPUOffloadingOMP)
//      #pragma omp end declare target
    #endif
  }
}


#endif

