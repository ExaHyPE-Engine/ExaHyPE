// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_ENUMERATOR_PATCH_ACCESS_H_
#define _EXAHYPE2_FV_ENUMERATOR_PATCH_ACCESS_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>


namespace exahype2 {
  namespace fv {
    namespace enumerator {
      /**
       * Patch access class
       *
       * This class is to be used by user code which do not only want to work
       * volume-wisely, but also need access to neighbour volumes of a voxel.
       * For example, it can be used in the source term code to get access to
       * the neighbour volumes to evaluate a second derivative.
       *
       * The class really is a wrapper around some pointer arithmetics. This
       * implies that you have to be careful when you use it, as it barely
       * enforces checks. If you use a halo layer of one for example and you
       * look two volumes to the left, then your code will likely be buggy, as
       * the outerleft volume within a patch does not have another volume left
       * of its left neighbour.
       *
       * The class can be read as a state-ful flavour of
       * AoSLexicographicVolumeEnumerator, as we always assume that the bare
       * patch data in ExaHyPE 2 is organised as AoS.
       *
       */
      struct PatchAccess {
        public:
          PatchAccess(const double* __restrict__ QIn, int numberOfVolumesPerAxisInPatch, int haloSize, int unknowns, int numberOfAuxiliaryVariables);


          std::string toString() const;

          int size() const;

          /**
           * Get a pointer to another voxel's data. Pass in {0,0,0} and 0 as
           * unknown, and you will get the current voxel for which you have
           * created the PatchAccess object. Pass in {0,0,0} and k to access
           * its kth unknown. Pass in {-1,0,0} for example to access the left
           * neighbour of a voxel.
           */
          const double* __restrict__ operator()( const tarch::la::Vector<Dimensions,int>&  relativeVolumePosition ) const;
          double operator()( const tarch::la::Vector<Dimensions,int>&  relativeVolumePosition, int unknown ) const;

          /**
           * Return access to the left neighbour. Left means left along coordinate
           * axis normal.
           */
          const double* __restrict__ left(int normal) const;
          const double* __restrict__ right(int normal) const;

          double centre(int unknown ) const;
          double left(int normal, int unknown ) const;
          double right(int normal, int unknown ) const;

        private:
          const double* __restrict__ _QIn;
          const int _numberOfVolumesPerAxisInPatch;
          const int _haloSize;
          const int _unknowns;
          const int _numberOfAuxiliaryVariables;
      };
    }
  }
}


#endif



