// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

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
      const peano4::datamanagement::CellMarker& marker, int numberOfDoFsPerAxisInPatch, int unknowns, double* values
    );

    /**
     * Restrict data by injection
     *
     * This routine works for an overlap of 1 or an overlap of 3k+2.
     * If the overlap equals 1, we walk along the fine grid layer
     * along the AMR boundary and we restrict every third finite
     * volume voxel.
     *
     * The routine restricts only only have to a face, and it
     * restricts the inner half:
     *
     * @image html Restriction_injection.png
     *
     * An interpolation sets the data in the halo layer of the fine
     * grid cell. For this, it uses interior data from the coarse
     * grid cell. A restriction sets the data in the halo of the
     * coarser cell. For this, it uses the data inside the finer
     * cell. In the sketch above, the left cell hosts the fine data.
     * The right cell is a coarse cell. An interior voxel from the
     * fine data left is copied into the halo of the coarser voxel.
     *
     * You can invert this behaviour by setting the marker.
     *
     *
     * ## Halo sizes
     *
     * If the overlap equals two, we don't take the voxel directly
     * adjacent to the resolution change face, but we take the one
     * that is one voxel further away, as the centre of this one
     * coincides with the centre of the coarser voxel. So we can
     * actually inject. A similar argument holds for an overlap of
     * 3+2.
     *
     * In all of these cases where we use a proper injection, i.e.
     * where the overlap is greater than one, we cannot befill the
     * whole coarser overlap. Instead, we will only fill one layer
     * for an overlap of 2, or two layers for an overlap of 5. The
     * remaining coarse layer entries are not touched.
     *
     * This means that the halo data of the coarse cell is potentially
     * incomplete - something to take into account if you use the data.
     */
    void restrictInnerHalfOfHaloLayer_AoS_inject(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues,
      bool                                      swapInsideOutside = false
    );

    /**
     * Restrict data by injection
     *
     * Required for dynamic AMR only. Invokes the one-sided routine twice:
     * for the inner and the outer half of the face.
     */
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
     * This routine should be used if a cell hosts two sets of unknowns
     *
     * Frequently used if your code hosts two PDE solvers. This routine
     * runs over all the dofs in destinationValues, computes their
     * position, rounds it to the closest dof within sourceValues, and
     * then copies the data over. This is, we assume that both the source
     * and destination field represent voxels (similar to a Finite Volume)
     * scheme or data which is hold within the centre of a subgrid
     * (totally staggered dof layout).
     *
     * The description above assumes that numberOfDoFsPerAxisInDestinationPatch<=numberOfDoFsPerAxisInSourcePatch.
     * While the routine also works if this is not the case, injecting
     * from a coarser mesh into a finer one introduces a huge numerical
     * inaccuracy (it is basically piece-wise constant) and you hence
     * might be better off with a linear restriction. If the inequality
     * is "violated", I'd however call this rather a projection and therefore
     * you have to search through the provided projection routines to find
     * such a linear scheme.
     */
    void restrictCellIntoOverlappingCell_inject(
      int     numberOfDoFsPerAxisInSourcePatch,
      int     numberOfDoFsPerAxisInDestinationPatch,
      int     unknowns,
      double* sourceValues,
      double* destinationValues
    );

    /**
     * Flavour of restrictCellIntoOverlappingCell_inject() where we inject the
     * solution but then take the average between the original value in
     * destinationValues an the injected value. This "damps" the impact of the
     * injection.
     *
     * @param weightOfInjectedValue If this value is 0.5, we take the average.
     *   If it equals 1.0, we end up exactly with restrictCellIntoOverlappingCell_inject(),
     *   i.e. overwrite the value in destinationValues. A value of 0.0 switches
     *   the injection off.
     */
    void restrictCellIntoOverlappingCell_inject_and_average(
      int     numberOfDoFsPerAxisInSourcePatch,
      int     numberOfDoFsPerAxisInDestinationPatch,
      int     unknowns,
      double* sourceValues,
      double* destinationValues,
      double  weightOfInjectedValue = 0.5
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
     * Restrict with averaging
     *
     * This routine works for overlaps of 1 and multiples of 3. However, it
     * does only set values for the overlap/3 adjacent cells. So if you work
     * with an overlap of 1, then the overlap just works fine, but it is not
     * really the average. It is the average of the one layer adjacent to the
     * fine grid transition. In 2d, it is the sum scaled with 1/3.
     *
     * @image html Restriction_averaging.png
     *
     * If you have an overlap of 3, the routine computes a meaningful average,
     * but as it has only three overlap cells on the fine grid, it can
     * only set the overlap cell 1 on the next coarser mesh.
     *
     */
    void restrictInnerHalfOfHaloLayer_AoS_averaging(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues,
      bool                                      swapInsideOutside = false
    );

    void restrictCell_AoS_tensor_product(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      const double* __restrict__ tangentialRestrictionMatrix1d,
      double* fineGridValues,
      double* coarseGridValues
    );

    /**
     * Restrict whole halo layer
     *
     * This routine is usually only called when we destroy a face completely. In
     * this case, we have to restrict the whole halo layer. Otherwise, we
     * typically only restrict the inner halo layer, i.e. half of the overall
     * face data.
     *
     * As we have a routine that handles half of the face data, I simply call
     * this routine twice, but once switch inside and outside.
     *
     * @see restrictInnerHalfOfHaloLayer_AoS_tensor_product()
     */
    void restrictHaloLayer_AoS_tensor_product(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__ normalRestrictionMatrix1d,
      const double* __restrict__ tangentialRestrictionMatrix1d,
      double* fineGridValues,
      double* coarseGridValues
    );

    /**
     * Restrict inner halo half of face data
     *
     *
     * Every cell has 2d faces. From a cell's perspective, each face hosts k
     * layers of the adjacent cell and k entries of the neighbouring cell.
     * The latter are usually called halo. If a face is a hanging face, these
     * halo data are interpolated and do not stem directly from a neighbour
     * cell (as there is no neighbour cell on the same resolution level). Once
     * the cell has updated its data, we have to befill the halo of the
     * adjacent coarser cell in return. This is what this routine is about.
     *
     * For this restriction, we take the inner data layers associated to the
     * face (therefore the name) and we restrict them into the halo, i.e.
     * outer face data, of the coarser cell.
     *
     * This routine assumes that the restriction can be written as a tensor
     * product between one operator along the face normal and an operator
     * describing the tangential restriction. The latter is applied d-1 times.
     *
     *
     * ## Normal restriction operator
     *
     * The normal restriction is constructed from a 1d adaptive mesh: The
     * operator describes how the k coarse grid values are initialised from
     * the 2k fine grid values:
     *
     * @image html Restriction_tensor_1d.png
     *
     * In the sketch above, we have a 1d adaptive mesh, where the coarse mesh
     * is on the left and the fine mesh on the right. We restrict from the
     * fine grid face (which is a point for a 1d setup) to the coarse grid
     * face. Peano works with multiscale meshes: So we have two cells on the
     * coarse mesh. The right cell is refined, i.e. overlaps
     * with a finer cell (fat lines bottom), while the left cell is unrefined.
     * The face in this example hosts an
     * overlap of two, i.e. copies of two entries left and right. I denote the
     * four coarse grid entries with green and blue bullets. The fine grid
     * unknowns associated with the face are denoted with red and orange dots.
     *
     * The restriction routine has to set the two coarse face entries to the
     * right, i.e. the blue values. The left entries (green) are initialised
     * properly by the mesh, as they stem directly
     * from an unrefined cell. You can also overwrite (add something to) the
     * left entries of the face, but most codes don't do so.
     *
     * The normal operator now describes through a 4x4 matrix, what data goes
     * from the fine mesh to the four coarser mesh. Two rows (for the green
     * dots) of this matrix are most of the time empty.
     *
     *
     * ### Overlap of one
     *
     * If you work with an overlap of one, the restriction along the normal is
     * kind of canonical. You can, for example, take the fine grid data (the
     * one orange point) and write that one to the one blue point.
     * In principle, the data flow is trivial.
     *
     * A more sophisticated scheme would set the blue value such that the
     * average in the real face position equals the average on the finer cell.
     * So we compute the average of red and orange, assume that the red point
     * equals hte value of the green point, and then set the blue point such
     * that the average between blue and green matches the other average.
     *
     *
     * ### Overlap greater than one
     *
     * With an overlap greater than one, we observe that the fine grid data
     * stops to overlap the coarse data spatially. We have discussed this
     * property by means of the injection (see
     * restrictInnerHalfOfHaloLayer_AoS_inject()). From the example above
     * where we illustrate an overlap of two, it becomes clear that we now
     * cannot set all halo data on the coarser mesh directly, i.e. using fine grid
     * data. The fine grid would hold all required data, but the routine
     * accepts the fine grid face data only. This data is not sufficient to set
     * all coarse entries.
     *
     * In the illustration, we can for example take the right orange point
     * and write its data to the left blue point. These two points do overlap.
     * However, we also have to set the right blue point, and there's no
     * fine grid info for this guy. The only thing that we can do is to take
     * the red and orange points, extrapolate somehow and then set the one
     * remaining blue point.
     *
     *
     * ## Tangential operator
     *
     * This yet has to be written.
     *
     *
     * ## Implementation
     *
     * We loop over a submanifold, i.e. over a d-1 dimensional array where the
     * index along the normal direction equals 0. The loop vector is called
     * kCoarse. iCoarse loops over the depth of the overlap, which we called k
     * in the examples above. The destination degree of freedom can be
     * constructed from kCoarse and iCoarse. For this, we have to take into
     * account that this iCoarse always counts from 0 to overlap, but
     * we obviously have to take into account if we consider a left hanging face
     * or a right hanging face and invert it accordingly. The destination index
     * determines the row that we use from the normal and tangential projection
     * matrices.
     *
     * We next construct the weight of the restriction and initialise it with
     * 1 as it will result from a tensor product of operators, i.e. a
     * multiplication. For the tensor-product, we run over each dimension. The
     * loop counter here's called k.
     *
     * Again, we do this combination of kFine and iFine, but iFine this time
     * runs over 2k elements. If we assess the normal operator, we have to
     * analyse if we work with a normalised layout, i.e. the left face of a
     * cell or the right one. If we try to find out the entry in the normal
     * matrix, we might have to mirror entries. All the other entries are
     * ordered along the coordinate axes.
     *
     *
     * ## Example matrices
     *
     * If we average over all fine grid values and set all coarse grid values to
     * the same restricted value, and if the overlap equals three, then you
     * would pass in the following matrix:
     *
     *        static constexpr double  NormalRestrictionMatrix1d[]     = {
     *          0.0,    0.0,    0.0,    1.0/3.0,    1.0/3.0,    1.0/3.0,
     *          0.0,    0.0,    0.0,    1.0/3.0,    1.0/3.0,    1.0/3.0,
     *          0.0,    0.0,    0.0,    1.0/3.0,    1.0/3.0,    1.0/3.0
     *        };
     *
     * The image below illustrates the data flow (for an overlap of two, but I
     * was too lazy to paint yet another pic):
     *
     * @image html Restriction_tensor_1d.png
     *
     * The three zeroes in the matrix mean that the real coarse grid values
     * (green) remain unchanged. The blue values are set according to the
     * three rows of the matrix. Each ignores the red entries to the left
     * (therefore the leading zeroes) and then computes the average of the
     * right ones. The result is then written to the coarse level.
     *
     * If you want to do the same type of averaging along the tangential, and
     * if you have a patch size of 5, you have to pass in the following matrix:
     *
     *       static constexpr double  TangentialRestrictionMatrix1d[] = {
     *         1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     *         0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     *         0, 0, 0, 0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 0, 0, 0,
     *         0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0,
     *         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0
     *       };
     *
     * We have 3x5 values on the fine grid, which determine the 5 entries on
     * the coarse mesh. Therefore, we have a 5x15 matrix. The first coarse mesh
     * entry is the average of the first three fine grid values. The second
     * coarse entry results from the fine grid values 4,5,6.
     *
     *
     * ## Arguments
     *
     * @param normalRestrictionMatrix1d Matrix that describes how the fine
     *   mesh unknowns affect the coarse grid unknowns. If you work with an
     *   overlap of k, normalRestrictionMatrix1d is a @f$ k \times 2k @f$
     *   matrix: It accepts the 2k fine grid values and spills out the value
     *   of the k outer coarse grid values.
     *
     *   The matrix refers to face number 0, or the sketch as given above.
     *   The values to restrict are the right ones (blue dots).
     *
     * @param swapInsideOutside All the discussions above refer to the
     *   initialisation of coarse grid's outer halo (blue points). You can
     *   alter this behaviour, i.e. make the routine manipulate the coarse
     *   grid green points instead, by setting the swap flag.
     *
     *
     *
     */
    void restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__ normalRestrictionMatrix1d,
      const double* __restrict__ tangentialRestrictionMatrix1d,
      double* fineGridValues,
      double* coarseGridValues,
      bool    swapInsideOutside = false
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
          tarch::la::Vector<Dimensions, int>    coarseVolume,
          tarch::la::Vector<Dimensions, int>    fineVolume,
          tarch::la::Vector<Dimensions, double> coarseVolumeCentre,
          tarch::la::Vector<Dimensions, double> fineVolumeCentre,
          double                                coarseVolumeH,
          double                                fineVolumeH
        )>                                        update
      );
    } // namespace internal
  }   // namespace blockstructured
} // namespace toolbox
