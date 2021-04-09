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

    /**
     * Copy the patch data from a patch representation with halo into
     * a representation without halo. If you set haloSize to zero, then
     * it is a direct memcopy.
     */
    void copyPatch(
      const double* __restrict__  QinWithHalo,
      double* __restrict__        QOutWithoutHalo,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize
    );

    
    double maxEigenvalue_AoS(
      std::function< double(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   eigenvalues,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__                  Q
    );
    

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
    void applySplit1DRiemannToPatch_Overlap1AoS2d(
      std::function< void(
        const double * __restrict__ QL,
        const double * __restrict__ QR,
        const tarch::la::Vector<2,double>&           faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<2,double>&           volueCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        double * __restrict__ S
      ) >   sourceTerm,
      const tarch::la::Vector<2,double>&           patchCentre,
      const tarch::la::Vector<2,double>&           patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );

    
    void applySplit1DRiemannToPatch_Overlap1AoS3d(
      std::function< void(
        const double * __restrict__ QL,
        const double * __restrict__ QR,
        const tarch::la::Vector<3,double>&           faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<3,double>&           volueCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        double * __restrict__ S
      ) >   sourceTerm,
      const tarch::la::Vector<3,double>&           patchCentre,
      const tarch::la::Vector<3,double>&           patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );


    void applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
      std::function< void(
        const double * __restrict__ QL,
        const double * __restrict__ QR,
        const tarch::la::Vector<2,double>&           faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<2,double>&           volueCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        double * __restrict__ S
      ) >   sourceTerm,
      const tarch::la::Vector<2,double>&           patchCentre,
      const tarch::la::Vector<2,double>&           patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );

    
    void applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
      std::function< void(
        const double * __restrict__ QL,
        const double * __restrict__ QR,
        const tarch::la::Vector<3,double>&           faceCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<3,double>&           volueCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        double * __restrict__ S
      ) >   sourceTerm,
      const tarch::la::Vector<3,double>&           patchCentre,
      const tarch::la::Vector<3,double>&           patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );


    
    /**
     * @param numberOfVolumesPerAxisInPatch That's the number of volumes in QOut, i.e.
     *        Qin has numberOfVolumesPerAxisInPatch-2*haloSizeAroundQin volumes per 
     *        axis.
     * @param copyPredicate Define per image volume whether to copy over or not, i.e. 
     *        argument in functor is position of image voxel (0<=i<numberOfVolumesPerAxisInPatch).
     */
    void copyPatch(
      const double* __restrict__  QinWithHalo,
      double* __restrict__        QOutWithoutHalo,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      std::function<bool(const tarch::la::Vector<Dimensions, int>&)>        copyPredicate
    );

    /**
     * Insert a (small) subpatch into another patch
     *
     * I assume that Qin is a subpatch inside QOut and that its values are to
     * be inserted. I use this routine for the optimistic tasking where I
     * compute partial results ahead of time and then have to insert them into
     * the solution later on.
     *
     * @param numberOfVolumesPerAxisInPatch That's the number of volumes in QOut, i.e.
     *        Qin has numberOfVolumesPerAxisInPatch-2*haloSizeAroundQin volumes per 
     *        axis.
     */
    void insertPatch(
      const double* __restrict__  Qin,
      double* __restrict__        QOut,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfVolumesPerAxisInPatch,
      int    haloSizeAroundQin
    );

    /**
     * A dimension-generic implementation of the update with guard
     *
     * This is a dimension-generic implementation of the patch update which
     * is realised in a cell-wise manner: I loop over the whole image patch
     * and solve 2d Riemann problems per cell. This is (usually) very
     * inefficient, as we solve each Riemann problem twice even though we
     * could take the solution and immediately pass it into two adjacent
     * cells. The implementation also exhibits no internal parallelism
     * compared to its dimension-specific peers which even exist in various
     * implementation variants.
     *
     * The big advantage is that this implementation has a guard predicate.
     * With the functor, you can determine which cells of the image are to
     * be updated. I use this routine to update the skeleton around a cell.
     */
    void applySplit1DRiemannToPatch_Overlap1AoS(
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
      std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&           volueCentre,
        double                                       volumeH,
        double                                       t,
        double                                       dt,
        double * __restrict__ S
      ) >   sourceTerm,
      const tarch::la::Vector<Dimensions,double>&           patchCentre,
      const tarch::la::Vector<Dimensions,double>&           patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout,
      std::function<bool(const tarch::la::Vector<Dimensions, int>&)>        updatePredicate
    );        

    /**
     * Compute maximum eigenvalue over selection of patch
     *
     * This is an extended alternative to maxEigenvalue_AoS(). It is less efficient
     * as I wrote it completely dimension-generic. However, it allows you to filter
     * out voxels, i.e. you can determine via the functor which volumes of the
     * image to update or not.
     *
     * @see maxEigenvalue_AoS()
     */
    double maxEigenvalue_AoS(
      std::function< double(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   eigenvalues,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__                  Q,
      std::function<bool(const tarch::la::Vector<Dimensions, int>&)>        analysePredicate
    );
  }
}


#endif

