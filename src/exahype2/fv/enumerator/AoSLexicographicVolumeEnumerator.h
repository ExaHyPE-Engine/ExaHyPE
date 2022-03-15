// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_ENUMERATOR_AOS_LEXICOGRAPHIC_VOLUME_ENUMERATOR_H_
#define _EXAHYPE2_FV_ENUMERATOR_AOS_LEXICOGRAPHIC_VOLUME_ENUMERATOR_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>


namespace exahype2 {
  namespace fv {
    namespace enumerator {
      /**
       * Default enumerator for volumes
       *
       * We assume that we have a lexicographic ordering of the patch data and
       * all data therein are ordered lexicographically and stored in as AoS.
       *
       * @param numberOfPatches_ This field is irrelevant for this enumerator,
       *   as the patches are enumerated one after another, i.e. there is no
       *   patch interleaving. To be formally correct, you should set it to 1
       *   if you construct and enumerator over the input or output data, as
       *   there is one patch per cell.
       */
      struct AoSLexicographicVolumeEnumerator {
        public:
          AoSLexicographicVolumeEnumerator(int numberOfPatches, int numberOfVolumesPerAxisInPatch, int haloSize, int unknowns, int numberOfAuxiliaryVariables):
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

          std::string toString() const;

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
    }
  }
}


#endif

