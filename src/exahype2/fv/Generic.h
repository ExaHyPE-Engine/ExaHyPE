// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_GENERIC_H_
#define _EXAHYPE2_FV_GENERIC_H_



#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>


namespace exahype2 {
  namespace fv {
    /**
     * Helper routine that I need in the log statements.
     *
     * @return Data of one volume as tuple.
     */
    std::string plotVolume(
      double Q[],
      int    unknowns
    );

    void copyPatch(
      double QinWithHalo[],
      double QOutWithoutHalo[],
      int    unknowns,
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize
    );

    namespace gpu {
      /**
       * This is a GPU version of the copy operator. It differs from the
       * default version as it writes out all loops explicitly has nested
       * loops. This became necessary as we can't use the d-dimensional
       * for macros in GPU code unfortunately.
       */
      #if defined(GPUOffloading)
      #pragma omp declare target
      #endif
      void copyPatch(
        double QinWithHalo[],
        double QOutWithoutHalo[],
        int    unknowns,
        int    numberOfVolumesPerAxisInPatch,
        int    haloSize
      );
      #if defined(GPUOffloading)
      #pragma omp end declare target
      #endif
    }

    /**
     * Is a face loop-based implementation of a split 1d Riemann.
     *
     * You are given two arrays: The reconstructed one and Qout. Qout has the
     * dimensions numberOfVolumesPerAxisInPatch x numberOfVolumesPerAxisInPatch.
     * The reconstructed patch Qin has the dimensions (numberOfVolumesPerAxisInPatch+1) x
     * (numberOfVolumesPerAxisInPatch+1). The data within the patch is stored as
     * AoS.
     *
     * Please consult the guidebook for some documentation of this routine. Some
     * background is given in its ExaHyPE chapter, but the really interesting
     * details are discussed within the Finite Volume chapter.
     */
    void applySplit1DRiemannToPatch_Overlap1AoS2d(
      std::function< void(
        double                                       QL[],
        double                                       QR[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      double                                       Qin[],
      double                                       Qout[]
    );


    void applySplit1DRiemannToPatch_Overlap1AoS3d(
      std::function< void(
        double                                       QL[],
        double                                       QR[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      double                                       Qin[],
      double                                       Qout[]
    );
  }
}


#endif

