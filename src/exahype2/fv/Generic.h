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
      const double* __restrict__ Q,
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
      const double* __restrict__ Q,
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
      const double* __restrict__  QinWithHalo,
      double* __restrict__        QOutWithoutHalo,
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
     * You are given two arrays: The Qin and Qout. Qout has the
     * dimensions numberOfVolumesPerAxisInPatch x 
     * numberOfVolumesPerAxisInPatch x (unknowns+aux variables). Qin has the 
     * dimensions (numberOfVolumesPerAxisInPatch+1) x
     * (numberOfVolumesPerAxisInPatch+1) x (unknowns + aux variables). The data
     * within both patches is stored as AoS. So the input data Qin is the patch
     * plus a halo of one cell. The output data is a patch only.
     * 
     * The routine runs over all faces within the input array and computes how 
     * much flows through these faces. It calls splitRiemannSolve1d() per face. 
     * This routine tells what flows into the left and into the right adjacent 
     * volume. This outflow is added to the respective volume in Qout. 
     *
     * @image html applySplit1DRiemannToPatch_Overlap1AoS2d.png
     *
     * <h2> Implementation </h2>
     * 
     * There are two big blocks. The first block runs over all faces with a 
     * normal along the x-direction. The second block over faces with normal 
     * along y-direction. For a 3x3 mesh as in the sketch above, the input 
     * patch has 5x5 volumes as we also get the halo. The output is only 3x3.
     * Each block is a nested for loop where one index runs over 0-2 and the
     * other one through four indices.
     *
     * We touch a face (see red one in sketch), e.g., and add the outcome to 
     * the volume of the left and right neighbour. Obviously, special care is 
     * required for the very left and very right face. They have a left and 
     * right adjacent volume in the preimage (Qin) which feeds into the Riemann 
     * solve, but the image hosts only the left or right volume.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    void applySplit1DRiemannToPatch_Overlap1AoS2d(
      std::function< void(
        const double * __restrict__ QL,
        const double * __restrict__ QR,
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
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif


    void applySplit1DRiemannToPatch_Overlap1AoS3d(
      std::function< void(
        const double * __restrict__ QL,
        const double * __restrict__ QR,
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
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );


    /**
     * The routine has exactly the same semantics as applySplit1DRiemannToPatch_Overlap1AoS2d()
     * but runs through the faces slightly differently. 
     *
     * @image html applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop.png
     *
     * As in applySplit1DRiemannToPatch_Overlap1AoS2d(), we have two big
     * blocks. The first one runs through all vertical faces, the second one 
     * through all horizontal ones. Each block now runs through the whole data
     * twice though with a spacing of two and an offset of 0 or 1. That is, 
     * we run through every second face (faces 0, 2, 4 along the x-axis) and 
     * then through the other ones (1,3,5,...). 
     *
     * Each face writes to its left and right adjacent volume in Qout. As we
     * skip every second face, we know that all writes are parallel. No race
     * conditions can arise. That is: Though we now run through the data structure
     * twice, we can process the for loops embarassingly parallel.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    void applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
      std::function< void(
        const double * __restrict__ QL,
        const double * __restrict__ QR,
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
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif


    void applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
      std::function< void(
        const double * __restrict__ QL,
        const double * __restrict__ QR,
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
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );
  }
}


#endif

