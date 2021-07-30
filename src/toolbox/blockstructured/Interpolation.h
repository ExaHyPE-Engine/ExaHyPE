// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_TOOLBOX_BLOCKSTRUCTURED_INTERPOLATION_H_
#define _PEANO4_TOOLBOX_BLOCKSTRUCTURED_INTERPOLATION_H_


#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"


namespace toolbox {
  namespace blockstructured {
    void clearHaloLayerAoS(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   values
    );


    /**
     * Take the coarse grid values and interpolate them onto the fine grid
     *
     * To understand the implementation of this routine, we have to reiterate
     * that we work cell-wisely in Peano. That is, we start from a particular
     * cell that has a father-cell. It is one out of 3^d child cells, as we
     * work with three-partitioning. Within this cell (which is encoded
     * within marker as we will discuss later), we have 2d faces. If one of
     * them is a hanging face (along an AMR transition), then this routine
     * will be called for it. The faces of any cell in Peano are enumerated
     * in the same way: Face 0 is the face whose normal points along the first
     * coordinate axis and which is left in the coordinate system. Face 1 is
     * the face whose normal points along the second coordinate axis, and so
     * forth. In 2d, face 0 is the left one, face 1 is the bottom one, face 2
     * is the right one, and face 3 finally the top face.
     *
     * We can ask the marker which face we are talking about via
     * getSelectedFaceNumber(). We can also ask the marker how this face is
     * positioned within the grid of siblings, i.e. within a 3x3x3 grid.
     * The routine for this is getRelativePositionWithinFatherCell().
     *
     * We run over all coarse grid volumes (counter k or coarseGridVolume,
     * respectively). Each coarse grid volume overlaps 1x3x3 or 3x1x3 or ...
     * smaller volumes. For both loops, I use the dfore (exclusive, d-dimensional
     * for).
     *
     * The marker has two interesting routines that are relevant for this
     * routine: getSelectedFaceNumber() tells us the which face we are
     * talking about from a fine
     *
     *
     */
    void interpolateOntoOuterHalfOfHaloLayer_AoS_piecewise_constant(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues,
      bool                                      swapInsideOutside=false
    );


    /**
     * Interpolate whole halo layer
     *
     * I originally thought I could work with interpolating only half of the
     * halo layer. Indeed, we only have to interpolate half of the halo layer
     * along a hanging face: the outer part. This outer data will be used to
     * construct a halo around the adjacent fine grid patch. However, I need
     * the interpolation of all data whenever I creat a new persistent face.
     * Threfore, there are two interpolation routines.
     */
    void interpolateHaloLayer_AoS_piecewise_constant(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );


    /**
     * This routine is called if we create a new cell (dynamic AMR)
     */
    void interpolateCell_AoS_piecewise_constant(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );


    void restrictOntoOuterHalfOfHaloLayer_AoS_averaging(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues,
      bool                                      swapInsideOutside=false
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


    void interpolateOntoOuterHalfOfHaloLayer_AoS_linear(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues,
      bool                                      swapInsideOutside=false
    );
    void interpolateHaloLayer_AoS_linear(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );
    void interpolateCell_AoS_linear(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
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
       * Helper routine - usually not invoked by user code
       *
       * Maps two halo layers onto each other, i.e. runs through both
       * layers through all NxMxM voxels, computes their centres, and
       * invokes the callback.
       *
       * Halo layers are associated with faces, and each face action is
       * called from within a cell, i.e. we run through the cells of the
       * spacetree and call touchFace.... or touchVertex... from the
       * cell's point of view. Each face has a number, and we can find
       * out via the selected face whether this is a left or right
       * face of a cell along a coordinate axis.
       *
       * @param marker Marker identifying which face it is from the
       *   corresponding cell's point of view.
       * @param mapInnerHalfOfHalo Take the inner half of the respective
       *   face's halo and map it up or down. If we pick the right half
       *   on the fine level, we also have to use the right half on the
       *   next coarser level.
       */
      void projectHaloLayers_AoS(
        const peano4::datamanagement::FaceMarker& marker,
        int                                       numberOfDoFsPerAxisInPatch,
        int                                       overlap,
        std::function<void(
          tarch::la::Vector<Dimensions,int> coarseVolume,
          tarch::la::Vector<Dimensions,int> fineVolume,
          tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
          tarch::la::Vector<Dimensions,double> fineVolumeCentre,
          double coarseVolumeH,
          double fineVolumeH
        )> update,
        bool mapInnerHalfOfHalo
      );


      /**
       * Helper function
       */
      void projectCells_AoS(
        const peano4::datamanagement::CellMarker& marker,
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
