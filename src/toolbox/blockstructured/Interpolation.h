// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_TOOLBOX_BLOCKSTRUCTURED_INTERPOLATION_H_
#define _PEANO4_TOOLBOX_BLOCKSTRUCTURED_INTERPOLATION_H_


#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"

#include "tarch/la/DynamicMatrix.h"


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


    void restrictOntoOuterHalfOfHaloLayer_AoS_inject(
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
     * The realisation relies on the following observations/follows these
     * steps:
     *
     * <h2> A 1d setup </h2>
     *
     * We first construct a 1d interpolation. Starting from the hard-coded pattern
     *
     * <pre>
    {1.0/3.0, 2.0/3.0,     0.0},
    {    0.0, 3.0/3.0,     0.0},
    {    0.0, 2.0/3.0, 1.0/3.0}
       </pre>
     *
     * we repeat this whole thing numberOfDoFsPerAxisInPatch times and each time
     * shift by one. For numberOfDoFsPerAxisInPatch=4 this yields for example
     *
     * <pre>
    {0.333333,0.666667,       0,0,0,0},
    {0,              1,       0,0,0,0},
    {0,       0.666667,0.333333,0,0,0},
    {0,       0.333333,0.666667,0,0,0},
    {0,              0,       1,0,0,0},
    ...
       </pre>
     *
     * Let this be the matrix P1d. If we had a 1d setup, we could compute
     *  fine = P1d * coarse
     * and would get all fine grid values in one rush. Obviously, we do not have a
     * 1d setup.
     *
     * <h2> 2d extensions </h2>
     *
     * Once we know P1d, we have to "blow is up". The input does not have
     * numberOfDoFsPerAxisInPatch entries but has numberOfDoFsPerAxisInPatch*2
     * entries. Also the image has to be multiplied by two. We fist determine the
     * normal and look whether we are left or right of this normal for the fine
     * grid face. Next, we reiterate that we work with a lexicographic enumeration.
     * If we want to study the left face of a cell,
     *
     *
     * <h2> The issue with the missing diagonal element </h2>
     *
     * We do not know the diagonal element which we'd need to interpolate the
     * outmost cells in our data. Without knowing them, two options are on the
     * table: We can interpolate constantly at the edges, or we can extrapolate.
     * While extrapolation might sound to be a good choice, I found that it
     * sometimes yields physically invalid results. The Euler equations, for
     * example, will immediately give you negative (tiny) energies or negative
     * pressure. So I disable any extrapolation by default. If you need it,
     * the following two lines would do the trick (see also commented out
     * fragments in source code);
     *
     * <pre>
  P1d(0,0) =  2.0/3.0 + 1.0/3.0 * 2.0;
  P1d(0,1) = -1.0/3.0;
  P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  2.0/3.0 + 1.0/3.0 * 2.0;
  P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) = -1.0/3.0;
     * </pre>
     *
     */
    template <int NumberOfDoFsPerAxisInPatch>
    void interpolateOntoOuterHalfOfHaloLayer_AoS_linear_precomputed_operators(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues,
      bool                                      swapInsideOutside=false
    );
    template <int NumberOfDoFsPerAxisInPatch>
    void interpolateHaloLayer_AoS_linear_precomputed_operators(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );
    template <int NumberOfDoFsPerAxisInPatch>
    void interpolateCell_AoS_linear_precomputed_operators(
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


    /**
     * Outflow AMR condition
     *
     * The idea here is that a wave travels from a fine grid resolution into
     * a coarse one. If the wave has a unique direction of travel, there should
     * be no reflections and we could use simple outflow conditions, i.e.
     * homogeneous Neumann.
     *
     * This does not work 1:1 in Peano, as hanging faces do not carry
     * information: Any hanging face/vertex is created throughout the traversal
     * and it is deleted towards the end. They are not persistent. So we can't
     * impose Neumann conditions: The fine grid patch projects its data onto the
     * inner layer of the face, but this projection is lost in the next sweep.
     *
     * So I have to create a face: I know that the next coarser face holds
     * meaningful data. I interpolate d-linearly onto the inner(!) halo layer,
     * i.e. those cells which should coincide with the patch content and are not
     * used along a hanging face. This is an approximation, as this layer should
     * hold copies of the patch interior. Now it holds a restricted plus
     * interpolated data set. With this approximation, I can now impose Neumann
     * conditions.
     */
    void interpolateOntoOuterHalfOfHaloLayer_AoS_outflow(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues,
      bool                                      swapInsideOutside=false
    );
    /**
     *  Interpolate whole edge
     *
     *  This is usually called if we create a new face, e.g. It is important
     *  that we use linear here, as outflow makes no sense if we set both
     *  layer halves.
     */
    void interpolateHaloLayer_AoS_outflow(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );
    void interpolateCell_AoS_outflow(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );


    namespace internal {
      /**
       * <h2> Insertion of empty lines/rows </h2>
       *
       * We assume that the result is "symmetric" - not in a linear equation system
       * sense, but in that the columns and row insertions always have to match.
       *
       *
       * @param number How many additional zero lines and columns are required?
       *              Set to -1 to disable any row/column insertion.
       * @see tarch::la::DynamicMatrix::insertColumns()
       */
      tarch::la::DynamicMatrix  create1dLinearInterpolation(int numberOfDoFsPerAxisInPatch, int number, int where, int repeat);

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


#include "Interpolation.cpph"

#endif
