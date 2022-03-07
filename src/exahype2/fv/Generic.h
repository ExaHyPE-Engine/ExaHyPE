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
     * Representation of a number of patches which contains all information
     * that's required to process them (given you know the semantics of the
     * data).
     *
     * This struct is a mixture of AoS and SoA. All data besides the Qin and
     * result are SoA. The reason for this "inconsistent" model is that we
     * want to transfer the patch data to accelerators quickly, while we also
     * want to avoid to copy too much stuff around. So we leave the big input
     * fields and basically manage just pointers to that. All scalar or small
     * vector data are converted in SoA which makes it fairly straightforward
     * later to move them over to an accelerator.
     *
     * @todo Doesn't belong into fv. The whole Generic header belongs into the
     * directory above.
     */
    struct PatchData {
      double**   QIn;
      tarch::la::Vector<Dimensions,double>*   patchCentre;
      tarch::la::Vector<Dimensions,double>*   patchSize;
      double*    t;
      double*    dt;
      int*       id;
      const int  numberOfPatches;
      double**   QOut;
      double*    maxEigenvalue;

      /**
       * Construct patch data object for one single cell.
       */
      PatchData(
        double*   QIn,
        const tarch::la::Vector<Dimensions,double>&   patchCentre_,
        const tarch::la::Vector<Dimensions,double>&   patchSize_,
        double    t,
        double    dt,
        double*   QOut
      );
      PatchData( int numberOfPatches_ );
      ~PatchData();
    } ;

    /**
     * Compute total number of unknowns (incl halo) for a 2d patch.
     */
    int getTotalNumberOfVolumeDoFs(
      int numberOfPatches,
      int numberOfVolumesPerAxisInPatch,
      int overlap
    );

    int getTotalNumberOfInnerVolumeDoFs(
      int numberOfPatches,
      int numberOfVolumesPerAxisInPatch,
      int overlap
    );

    int getTotalNumberOfFaceDoFs(
      int numberOfPatches,
      int numberOfVolumesPerAxisInPatch,
      int overlap
    );

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
          ) const {
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

          const int _numberOfPatches;
          const int _numberOfVolumesPerAxisInPatch;
          const int _haloSize;
          const int _unknowns;
          const int _numberOfAuxiliaryVariables;
      };

    }



    //
    // ====================
    //         old
    // ====================
    //



    /**
     * Copy the patch data from a patch representation with halo into
     * a representation without halo. If you set haloSize to zero, then
     * it is a direct memcopy.
     *
     * It is important to recognise that this routine does also copy the
     * auxiliary values.
     */
    void copyPatch(
      const double* __restrict__  QinWithHalo,
      double* __restrict__        QOutWithoutHalo,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize
    );


    // @todo raus
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
     *
     *        //  @todo raus
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
     *
     *        // @todo raus
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

