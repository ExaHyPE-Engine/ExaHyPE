// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ENUMERATOR_AOSOA_LEXICOGRAPHIC_ENUMERATOR_H_
#define _EXAHYPE2_ENUMERATOR_AOSOA_LEXICOGRAPHIC_ENUMERATOR_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>


namespace exahype2 {
  namespace enumerator {
    /**
     * This enumerator enumerates the patches one after the other (AoS). Within each
     * patch, it then however switches to an SoA data layout.
     */
    #if defined(GPUOffloadingOMP)
//      #pragma omp declare target
    #endif
    struct AoSoALexicographicEnumerator {
      public:
        AoSoALexicographicEnumerator(int numberOfPatches, int numberOfVolumesPerAxisInPatch, int haloSize, int unknowns, int numberOfAuxiliaryVariables):
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
         */
        int operator()(
          int patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          int unknown
        ) const InlineMethod {
          if (Dimensions==2) {
            return (patchIndex % _numberOfPatches) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
                 + unknown * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
                 + (volumeIndex(1)+_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
                 + (volumeIndex(0)+_haloSize);
          }
          else {
            return (patchIndex % _numberOfPatches) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
                 + unknown * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
                 + (volumeIndex(2)+_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
                 + (volumeIndex(1)+_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
                 + (volumeIndex(0)+_haloSize);
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

