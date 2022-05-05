// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_BLOCKSTRUCTURED_RESTRICTION_H_
#define _TOOLBOX_BLOCKSTRUCTURED_RESTRICTION_H_

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"

#include "tarch/la/DynamicMatrix.h"


namespace toolbox {
  namespace blockstructured {
    /**
     * Clear halo layer of face
     *
     * Hand in a face data structure and its marker. From the marker, we know
     * which half of the face data is actual the halo: If we are talking about
     * face 0, i.e. the left face of a cell, then the left half of the volumes
     * within the face are to be clared. If we are getting face 1, which is the
     * one at the bottom, it is the lower layer of volumes within the face data
     * which has to be cleared.
     *
     * Clearing the halo layer is something that we have to do whenever we create
     * a new face. In theory, we could skip this, as a proper interpolation will
     * eventually set meaningful values, but I found it useful to have non-garbage
     * within the faces.
     *
     * We also have to clear the layer prior to any restriction, as restrictions
     * typically accumulate a result (while an interpolation just writes the
     * interpolated value to a cell). The reason is as follows: When you interpolate,
     * you have the coarse data and you can interpolate the whole value in one rush.
     * When you restrict, you typically assemble the final restricted value step by
     * step as you run through the finer cells. But there's no point where you have
     * all the fine grid data on the table at the same time. So we have to accumulate.
     * Clearing is something most codes do in touchFaceFirstTime(). Most codes also
     * work with two types of face data: There's a backup of the face data into which
     * they write the current data. This is then used for interpolation, while the
     * actual data is cleared and used to accumulate the restricted values.
     */
    void clearHaloLayerAoS(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   values
    );


    void clearCell(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      double*                                   values
    );


    /**
     * This routine is used when we delete a cell due to dynamic AMR
     */
    void restrictCell_AoS_averaging(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );


    void restrictInnerHalfOfHaloLayer_AoS_inject(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues,
      bool                                      swapInsideOutside=false
    );


    void restrictHaloLayer_AoS_inject(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );


    void restrictCell_AoS_inject(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );


    /**
     * Consult commend on interpolation that clarifies why we need two
     * different halo layer restrictions, i.e. one for half of the
     * halo and one for the whole thing.
     */
    void restrictHaloLayer_AoS_averaging(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );


    void restrictInnerHalfOfHaloLayer_AoS_averaging(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues,
      bool                                      swapInsideOutside=false
    );

    namespace internal {


      /**
       * Clear half of a halo layer
       *
       * Faces in blockstructured codes with patches of size NxNxN typically
       * carry overlaps of size MxNxN. The M is an even number with M<2N. If
       * you use overlaps/halos of size 1, then M=2: a face manages one layer
       * from the left and one layer from the right.
       *
       * In blockstructured codes, you often have to erase half of this halo
       * layer. If you run into an adaptive grid, for examples, you want to
       * erase hanging layers before you interpolate or restrict into them.
       * This is what this routine does.
       *
       * @param marker    This marker identifies a face. See the class documentation
       *   in particular for details about the enumeration (selected faces) of the
       *   faces from a cell's point of view.
       * @param numberOfDoFsPerAxisInPatch The N in the description above
       * @param overlap   Equals M/2 in the description above, i.e. if you each
       *   patch is surrounded by one halo layer, then the total overlap equals
       *   two and this argument equals 1.
       * @param unknowns  Number of unknowns that we store per voxel.
       * @param clearOuterPart If this flag is set, we clear the outer half of
       *   the overlap (relative to the normal identified by marker).
       */
      void clearHalfOfHaloLayerAoS(
        const peano4::datamanagement::FaceMarker& marker,
        int                                       numberOfDoFsPerAxisInPatch,
        int                                       overlap,
        int                                       unknowns,
        bool                                      clearInnerPart,
        double*                                   values
      );


      /**
       * Helper function
       *
       * This function runs through all fine grid/coarse grid cell combinations which
       * are identified via fineGridCellMarker. So we call the functor once per fine
       * grid cell, but @f$ 3^d @f$ times or not at all for any coarse grid cell. This
       * routine is used as a helper function for piece-wise constant interpolation.
       * It is not that useful for d-linear interpolation.
       */
      void projectCells_AoS(
        const peano4::datamanagement::CellMarker& fineGridCellMarker,
        int                                       numberOfDoFsPerAxisInPatch,
        std::function<void(
          tarch::la::Vector<Dimensions,int> coarseVolume,
          tarch::la::Vector<Dimensions,int> fineVolume,
          tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
          tarch::la::Vector<Dimensions,double> fineVolumeCentre,
          double coarseVolumeH,
          double fineVolumeH
        )> update
      );
    }
  }
}


#endif
