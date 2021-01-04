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

    /**
     * Just runs over the patch and ensures that no entry is non or infinite. In
     * ExaHyPE, we primarily work with split approaches. That is, diagonal halo
     * entries are never initialised properly: We can copy over the face-connected
     * data, but we lack information through the diagonals. This routine takes
     * this into account when it validates the entries.
     *
     * @param location String that tells system from where this routine got called
     */
    void validatePatch(
      double* __restrict__ Q,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      const std::string& location = ""
    );

    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    void copyPatch(
      double* __restrict__ QinWithHalo,
      double* __restrict__ QOutWithoutHalo,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif

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
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    void applySplit1DRiemannToPatch_Overlap1AoS2d(
      std::function< void(
        double * __restrict__ QL,
        double * __restrict__ QR,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ Qin,
      double * __restrict__ Qout
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif


    void applySplit1DRiemannToPatch_Overlap1AoS3d(
      std::function< void(
        double * __restrict__ QL,
        double * __restrict__ QR,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ Qin,
      double * __restrict__ Qout
    );


    void applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
      std::function< void(
        double * __restrict__ QL,
        double * __restrict__ QR,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ Qin,
      double * __restrict__ Qout
    );


    void applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
      std::function< void(
        double * __restrict__ QL,
        double * __restrict__ QR,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ Qin,
      double * __restrict__ Qout
    );
  }
}


#endif

