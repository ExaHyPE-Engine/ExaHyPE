// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"

#include "tarch/la/DynamicMatrix.h"

#include "peano4/utils/Loop.h"


#include <map>

/**
 * @page toolbox_blockstructured_interpolation Halo layer interpolation
 *
 * When we create hanging faces, we have to interpolate from coarse to
 * fine.
 *
 * ## Data storage scheme
 *
 * For a setup with p volumes per coordinate axis per patch, a
 * face always stores @f$ 2k \cdot p^{d-1} @f$ values: It stores a layer of
 * depth k of its left neighbouring cell and a layer of the right one. These layers
 * are copies. By separating the copies of halo layers from the actual
 * patch data, we can easier move data around in a parallel computer. A
 * patch only has to know some of the data from all neighbouring, face-connected
 * patches. With our scheme, it does not need to know these neighbours at
 * all. It is sufficient to take the faces of a cell and to use the data
 * from there to reconstruct the actual patch plus its halo layer:
 *
 * @image html Interpolation00.png
 *
 * In the image above, we see a schematic illustration of the data layout:
 * In this example, a 2d cell hosts a 5x5 grid of finite volumes and we work
 * with an overlap of k=1. The four
 * faces each hold 2x5 volumes. These are copies from the left and right
 * adjacent cells. So the dark red volumes are actually copies of the volumes
 * within the patch. In the left image, I use a darker grey to label those
 * volumes within the patch which also have been copied by the patch onto
 * the face.
 *
 * So the four faces hold replicated data from their adjacent cells. When we
 * want to run the Rusanov solver, e.g., we need information from the neighbouring
 * cells. Peano does not directly use the neighbours but instead uses the
 * replicated data from the faces to supplement the neighbour information.
 * In this illustration, the dark data are used to re-construct the halo.
 * Only half of the face data is used for this reconstruction. The other half
 * is used by the respective other neighbours.
 *
 * When we now look at hanging faces, we recognise that we need interpolated
 * data from the coarser level:
 *
 * @image html Interpolation01.png
 *
 * Hanging faces do not carry any information, i.e. there is no red data in
 * the plots on the right. Only the face on the coarser level holds data.
 * So we have to project the light red volumes down to the fine grid light
 * red volumes, before we can update the fine grid patches (grey).
 *
 * The Interpolation.h header file hosts a set of interpolation routines.
 *
 *
 * ## Finding the grid topology
 *
 * To understand the implementation of the routines, we have to reiterate
 * that we work cell-wisely in Peano. That is, we start from a particular
 * cell that has a father-cell. It is one out of 3^d child cells, as we
 * work with three-partitioning. Within this cell (which is encoded
 * within marker as we will discuss later), we have 2d faces. If one of
 * them is a hanging face (along an AMR transition), Peano
 * will call an interpolation routine for it. The faces of any cell in Peano are enumerated
 * in the same way: Face 0 is the face whose normal points along the first
 * coordinate axis and which is left in the coordinate system. Face 1 is
 * the face whose normal points along the second coordinate axis, and so
 * forth. In 2d, face 0 is the left one, face 1 is the bottom one, face 2
 * is the right one, and face 3 finally the top face.
 *
 * We can ask the marker which face we are talking about via
 * getSelectedFaceNumber(). We can also ask the marker how this face is
 * positioned within the grid of siblings, i.e. within a 3x3x3 grid.
 * The routine for this is getRelativePositionWithinFatherCell(). In the
 * example above, this routine would return (0,0), (0,1) or (0,2), respectively,
 * Once we know the local face number and this position within 3x3x3 grid,
 * we know the exact grid topology of a hanging face.
 *
 * I originally thought I could work with interpolating only half of the
 * halo layer. Indeed, we only have to interpolate half of the halo layer
 * along a hanging face: the outer part. This outer data will be used to
 * construct a halo around the adjacent fine grid patch. However, I need
 * the interpolation of all data whenever I create a new persistent face,
 * restricting to half of the halo does not make the code any simpler, and
 * along the domain boundary, e.g., some routines appreciate if they have
 * all interpolation available.
 *
 * So while I could, in most cases, use the information of the normal to
 * only write half of the halo data (for example only the bright red volumes
 * on the fine mesh in the sketch above), I usually set all the face data
 * on the hanging face, even though half of it is not used.
 *
 *
 * ## Face data enumeration
 *
 * All face data are always enumerated lexicographically. That is, 0 is always
 * the left bottom volume within the face data. Then we follow the x-axis, then
 * the y-axis.
 *
 * The image below shows the enumeration:
 *
 * @image html Interpolation02.png
 *
 *
 * ## Interpolation as matrix-vector product
 *
 * We can always interpret the interpolation as a matrix-vector product. For
 * a system with N unknowns, it is important to recognise that we basically
 * do the same interpolation N times. This is a batched operation, i.e. we compute
 *
 * @f[  u_h = P u_H @f]
 *
 * for N versions of @f$ u_h @f$ and @f$ u_H @f$. Peano's tarch component has a routine
 * for batched multiplication, where you pipe in a set of @f$ u_H @f$ vectors and you
 * get a set of @f$ u_h @f$ vectors out.
 *
 *
 * ## Interpolation flavours
 *
 * Each interpolation type is realised via three different routes. All flavours follow
 * the same naming convention.
 *
 * - Each routine starts with the prefix interpolate.
 * - There's always one flavour that's then called interpolateCell, the other one is
 *   called interpolateHaloLayer.
 * - I add the data storage format that underlies the
 *   operator realisation. At the moment, I only support array of struct (AoS).
 * - The storage format identifier is followed by another underscore.
 * - The next part of the string is the actual interpolation scheme.
 *
 * We offer two types of interpolation: There's a few hard-coded interpolation schemes
 * which are written in core C++ and realise the interpolation. There is also a generic
 * verison of the interpolation which expects the user to pass in an interpolation
 * matrix and then applies this matrix instead of a hard-coded operation.
 *
 * If you work with the generic implementation accepting matrices, then most codes
 * pre-compute the required matrices manually (within a Python script, e.g.) and then
 * pass in a pointer to a constant double field holding the matrix. Please consult
 * toolbox::blockstructured::interpolateHaloLayer_AoS_generic() for details.
 *
 * Technically, there are always two types of interpolation routines per face:
 * There is a version which is given the coarse cell face only. I use this one to interpolate
 * from a coarse level onto a face at the boundary of a finer @f$ 3 \times 3 @f$
 * patch. The other variant of the interpolation is used for an interior face within
 * a @f$ 3 \times 3 @f$  patch and is also given the coarse cell data. The latter
 * variant is only called to create new faces. It is never used for hanging faces.
 *
 * For this one, I have to create the fine grid volumetric data, and then copy the
 * interpolated data from there onto the face data. This way, we are consistent
 * with the idea that face data always holds copies from adjacent fine grid data.
 *
 *
 * ## Caching of inter-grid transfer operators (only for built-in C++ operators)
 *
 * Constructing the interpolation operators can be time consuming. I therefore cache
 * the operators.
 *
 * For the handling of faces at the boundary of the @f$ 3 \times 3 \times 3 @f$ patches,
 * I hold one large interpolation matrix per normal. So for 2d, I have four interpolation
 * matrices, for 3d I have six. The large interpolation matrix per direction can compute
 * all @f$ 3^{d-1} @f$ faces in one rush, so we usually only need one out of them at a time.
 * So I have this huge map with these matrices. When we have to interpolate, we first
 * check whether we've already assembled the matrix. If so, we use this one. If not, we
 * first compute it and cache it in the map.
 *
 * For the volumetric interpolation and the handing of the interior faces, we use another
 * map. That is, if we want to compute the interior face, we first project onto a temporary
 * @f$ 3^d @f$ set of volumes, and then we extract the fine face data from there. This
 * implies that we are consistent with the actual projection of face data, where we take
 * fine grid volume values and write them onto the face data as copies.
 *
 *
 */
namespace toolbox {
  namespace blockstructured {
    /**
     * Take the coarse grid values and interpolate them onto the fine grid
     *
     * This routine expects two matrices. Both phrase 1d interpolation
     * rules, but one of them formalises the interpolation along the
     * face normal and one the interpolation along the tangential directions.
     *
     * The normal interpolation is a matrix of size overlap x 2*overlap. If
     * I have an overlap of 1, then I have to befill one halo layer around
     * a resolution transition.
     *
     *
     * ## Interpolation along the normal direction
     *
     * @image html Interpolation_tensor_1d.png
     *
     * The interpolation along the normal is a matrix of the dimensions
     * @f$ 2k \times k @f$. It is always defined along the left-to-right,
     * i.e. what happens if we have a face where the grid right of it is
     * refined and the grid left of it is unrefined. In this case, we have
     * two faces (one for the coarse level and one for the finer level).
     *
     * The image above illustrates this for k=2. In this case, the coarse
     * face holds four entries (along the normal): The green ones to the
     * left are real coarse grid data stemming from the adjacent block, while
     * the blue ones are overlapping with the fine mesh and therefore likely
     * restricted data. For AMR, we have to set (interpolate) the halo on the
     * fine level, i.e. the red values. We do not have to interpolate the
     * orange values, as these guys hold proper fine grid data. So that leaves
     * us with two entries on the fine mesh, which can be initialised using the
     * four entries on the coarser mesh. Overall, the
     * interpolation can be written down as 2x4 matrix.
     *
     * The illustration arrows above show the data flow for the left halo
     * entry.
     * This one can be befilled by four entries on the coarser resolution,
     * though only two of them are real entries and the other ones are
     * restricted onvalues. So you can either have the corresponding restricted
     * value entry in normalInterpolationMatrix1d as 0 - in this case you
     * only couple from real coarse data to fine data - or you can have
     * a value in there which means you interpolate over the resolution
     * transition between real and restricted data.
     *
     *
     * ## Tangential interpolation matrix
     *
     * The normalInterpolationMatrix1d has the size 3 * numberOfDoFsPerAxisInPatch
     * x numberOfDoFsPerAxisInPatch. So we have three matrix. Each of them
     * phrases how an element of one of the three fine grid segments is
     * affected by the numberOfDoFsPerAxisInPatch counterparts on the
     * coarser level.
     *
     *
     * ## Example matrices
     *
     * ### Piece-wise interpolation
     *
     * The interpolation matrix below realises a piece-wise constant interpolation
     * along the normal for an overlap of three. It takes the coarse grid value of
     * the voluem that's one away from the AMR boundary and interpolates it to the
     * fine mesh:
     *
     *         static constexpr double  NormalInterpolationMatrix1d[]     = {
     *            0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
     *            0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
     *            0.0, 1.0, 0.0, 0.0, 0.0, 0.0
     *         };
     *
     * The data flow is illustrated below:
     *
     * @image html Interpolation_tensor_1d_constant_interpolation.png
     *
     * Most constant interplation schemes would obviously not take the middle green
     * value but the one right of it. In the matrix, this means that third column
     * would be 1 and not the second one.
     *
     * If you also want to interpolate piece-wise constant along the tangential
     * directions, and if you have a patch size of 5, you would pass in the following
     * tangential interpolation matrix:
     *
     *       static constexpr double  TangentialInterpolationMatrix1d[] = {
     *         1.0,0.0,0.0,0.0,0.0,
     *         1.0,0.0,0.0,0.0,0.0,
     *         1.0,0.0,0.0,0.0,0.0,
     *         0.0,1.0,0.0,0.0,0.0,
     *         0.0,1.0,0.0,0.0,0.0,
     *
     *         0.0,1.0,0.0,0.0,0.0,
     *         0.0,0.0,1.0,0.0,0.0,
     *         0.0,0.0,1.0,0.0,0.0,
     *         0.0,0.0,1.0,0.0,0.0,
     *         0.0,0.0,0.0,1.0,0.0,
     *
     *         0.0,0.0,0.0,1.0,0.0,
     *         0.0,0.0,0.0,1.0,0.0,
     *         0.0,0.0,0.0,0.0,1.0,
     *         0.0,0.0,0.0,0.0,1.0,
     *         0.0,0.0,0.0,0.0,1.0
     *       };
     *
     * This operator is self-explaining: With a patch size of 5, we have five
     * coarse grid values along a tangent which are to projected onto 3x5 fine
     * grid values, as we always split into three. The operators are always
     * read along the coordinate axes. The left fine grid value has to equal
     * the left-most coarse grid value. This is the 1 in the first row of the
     * matrix. The second entry on the fine grid also holds this value. This is
     * the second line.
     */
    void interpolateHaloLayer_AoS_tensor_product(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                normalInterpolationMatrix1d,
      const double* __restrict__                tangentialInterpolationMatrix1d,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );


    void interpolateHaloLayer_AoS_tensor_product(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                normalInterpolationMatrix1d,
      const double* __restrict__                tangentialInterpolationMatrix1d,
      const double* __restrict__                coarseGridCellValues,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );


    void interpolateCell_AoS_tensor_product(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      const double* __restrict__                interpolationMatrix1d,
      const double* __restrict__                coarseGridCellValues,
      double* __restrict__                      fineGridCellValues
    );


    /**
     * Take the coarse grid values and interpolate them onto the fine grid
     */
    void interpolateHaloLayer_AoS_piecewise_constant(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );

    /**
     * This is the routine for creating a new persistent face
     *
     * Normal hanging faces are always created along the boundary of a 3x3 or
     * 3x3x3 patch, respectively. Therefore, we always take the coarse grid
     * face data to initialise the face values. These face data hold the overlaps,
     * so we can interpolate accordingly. Therefore, if marker.isInteriorFaceWithinPatch()
     * does not hold, we can use the alternative interpolateHaloLayer() variant.
     *
     * If we create persistent faces within a 3x3x3 patch arrangement, we first
     * create the interpolation of the left and right patch of the face of interest.
     *
     */
    void interpolateHaloLayer_AoS_piecewise_constant(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );


    /**
     * This routine is called if we create a new cell (dynamic AMR)
     *
     * The routine looks up if the interpolation matrix does exist already. If not,
     * it creates it. Afterwards, it takes the interpolation matrix and multiplies
     * it with the coarse matrix. Interpolation matrices are band matrices and hold
     * all entries for all @f$ 3^d @f$ subpatches. We only need one subpatch, so we
     * pick a subsegment of the matrix. In return, we use the same interpolation
     * scheme for all unknowns, so we can rely on a batched matrix multiplication.
     */
    void interpolateCell_AoS_piecewise_constant(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      double* __restrict__                      fineGridCellValues
    );


    void interpolateHaloLayer_AoS_linear_with_constant_extrapolation(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );

    void interpolateHaloLayer_AoS_linear_with_constant_extrapolation(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );

    void interpolateCell_AoS_linear_with_constant_extrapolation(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      double* __restrict__                      fineGridCellValues
    );


    void interpolateHaloLayer_AoS_linear_with_linear_extrapolation(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );

    void interpolateHaloLayer_AoS_linear_with_linear_extrapolation(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );

    void interpolateCell_AoS_linear_with_linear_extrapolation(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      double* __restrict__                      fineGridCellValues
    );


    /**
     * @see createLinearInterpolationMatrix()
     */
    void interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );

    void interpolateHaloLayer_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );

    void interpolateCell_AoS_linear_with_constant_extrapolation_and_linear_normal_interpolation(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      double* __restrict__                      fineGridCellValues
    );

    void interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );

    void interpolateHaloLayer_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    );

    void interpolateCell_AoS_linear_with_linear_extrapolation_and_linear_normal_interpolation(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      double* __restrict__                      fineGridCellValues
    );

    /**
     * This interpolation should be used if a cell hosts two sets of unknowns
     *
     * Frequently used if your code hosts two PDE solvers. In principle, it is
     * up to the user to decide if they want to interpolate from pure cell data
     * or use a reconstructed patch, i.e. cell data plus halo. However, if the
     * finer cell has more degrees of freedom than the source, then any
     * interpolation without halo data will yield wrong results close to the
     * boundary of the patch.
     *
     * Here's some ASCII art to illustrate this:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * |------|------|------|------|------|------|
     * |--|--|--|--|--|--|--|--|--|--|--|--|--|--|
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * Inside the domain, all interpolation is straightforward. However, we do
     * not have valid data for the very left and very right volume on the fine
     * grid. Such data are only available once we know the halo data, too.
     *
     *
     * ## Data validity
     *
     * There is one important catch when you interpolate linearly from a
     * coarse patch into a fine one: In ExaHyPE 2, we typically only have
     * face-connected data. Therefore, those coarse grid entries along the
     * patch diagonals are invalid, and we have to ignore them. Simply leaving
     * them away however is not an option, as we then would accumulate partial
     * contributions which do not sum up to 1.0.
     *
     *
     * ## Optimisation
     *
     * This routine can be quite time consuming, which is annoying if it is
     * used for volumetric coupling. ExaHyPE's limiting is such a process.
     * I therefore tried to optimise the routine in various ways:
     *
     * - The dfor loops in general cannot be vectorised. So I replaced the
     *   inner one with nested loops - hoping that this would allow the
     *   compiler to vectorise. However, it does not do so. This is likely
     *   due to the fact that there is an accumulation in there.
     * - Intel's vectorisation reports claim that the loop manipulating
     *   outsidePatchAlongCoordinateAxis is not worth vectorising. I tried
     *   the variant where I comment out some lines. This does not work. It
     *   still claims it were not worth it.
     * - I replaced the destination dfor loop with nested normal for loops
     *   and added a collapse and parallel statement (for OpenMP). This
     *   parallelisation is valid, as we split up the destination index and
     *   hence do not introduce any race conditions when we accumulate the
     *   destination value. However, I have to use an OpenMP taskloop to
     *   parallelise the code, as this routine is usually used within tasks,
     *   i.e. a parallel for would be tied to the single core that's given
     *   to the task.
     *
     *
     * @param haloSourcePatch
     * @param haloDestinationPatch Note that we only interpolate into the
     *   interior of the patch correctly.
     */
    void interpolateCellDataAssociatedToVolumesIntoOverlappingCell_linear(
      int                                       numberOfDoFsPerAxisInSourcePatch,
      int                                       numberOfDoFsPerAxisInDestinationPatch,
      int                                       haloSourcePatch,
      int                                       haloDestinationPatch,
      int                                       unknowns,
      const double* __restrict__                sourceValues,
      double* __restrict__                      destinationValues,
      ::peano4::utils::LoopPlacement          parallelisation
    );

    void interpolateCellDataAssociatedToVolumesIntoOverlappingCell_fourthOrder(
      int                                       numberOfDoFsPerAxisInSourcePatch,
      int                                       numberOfDoFsPerAxisInDestinationPatch,
      int                                       haloSourcePatch,
      int                                       haloDestinationPatch,
      int                                       unknowns,
      const double* __restrict__                sourceValues,
      double* __restrict__                      destinationValues,
      ::peano4::utils::LoopPlacement          parallelisation
    );

    namespace internal {
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
       * pressure.
       *
       * @image html Interpolation_createLinearInterpolationMatrix.png
       *
       * The picture above illustrates the problem: We have by default no access to
       * diagonal voxels in the mesh. This missing piece of information is illustrated
       * by a voxel which is crossed out. If we now map the bright red voxels onto the
       * bright green voxels, we don't really know what to do with the outermost
       * voxels: We can extrapolate constantly or linearly, but we will always be
       * slightly wrong, as we lack the information from the neighbour.
       *
       * Extrapolating constantly seems to be pretty robust, but actually makes our
       * interpolation scheme deterioriate towards something in-between piece-wise
       * constant and linear. Extrapolating linearly is better, but might yield
       * unphysical solutions. For example, assume there is a density shock with a
       * steep drop between the two outermost voxels. If we extrapolate linearly,
       * we might end up with an interpolated/extrapolated fine grid voxel with
       * negative density. This yields a wrong solution. To fix such a case, you
       * might want to work with linear extrapolation but also use a postprocessing
       * (clean-up) step. See the guidebook for an explanation.
       */
      tarch::la::DynamicMatrix*  createLinearInterpolationMatrix(
        int   numberOfDoFsPerAxisInPatch,
        int   normal,
        bool  extrapolateLinearly,
        bool  interpolateLinearlyAlongNormal
      );

      /**
       * This is a volumetric version of the interpolation.
       */
      tarch::la::DynamicMatrix*  createLinearInterpolationMatrix(int numberOfDoFsPerAxisInPatch, bool extrapolateLinearly);

      tarch::la::DynamicMatrix*  createLinearInterpolationMatrix(
        int numberOfDoFsPerAxisInInputPatch,
        int numberOfDoFsPerAxisInOutputPatch,
        bool extrapolateLinearly
      );

    /**
     * Create piecewise constant interpolation matrix
     *
     * The matrix is very simple as it holds exclusively zeroes and ones. To
     * construct it, we run over the image and and look for each image voxel
     * which coarser face voxel is overlaps. In this context, we exploit the
     * fact that all voxels are enumerated lexicographically:
     *
     * @image html Interpolation_createPiecewiseConstantInterpolationMatrix.png
     *
     * The resulting matrix for the example above (where the normal equals 0)
     * is 24x8 matrix which equals
     *
     * \f$
    \begin{array}{cccccccc}
    1 & 0 & 0 & 0 & 0 & 0 & 0 & 0 \\
    0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 \\
    1 & 0 & 0 & 0 & 0 & 0 & 0 & 0 \\
    0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 \\
    1 & 0 & 0 & 0 & 0 & 0 & 0 & 0 \\
    0 & 1 & 0 & 0 & 0 & 0 & 0 & 0 \\
    0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 \\
    0 & 0 & 0 & 1 & 0 & 0 & 0 & 0 \\
    0 & 0 & 1 & 0 & 0 & 0 & 0 & 0 \\
    0 & 0 & 0 & 1 & 0 & 0 & 0 & 0
    \end{array}
      \f$
     *
     * Below is another example for a face on y-direction in a 2D setup, assuming three-partitioning for every patch.
     * when we refine a face, we get @f$ 3^{d-1} @f$ subfaces.
     * we enumerate the volumes within a surface first, and then move to the next surface lexicographically.
     *
     * @image html Interpolation_ydirection_example.png
     *
     * ## Construction algorithm
     *
     * I have two variants of the actual construction in here. One of them is
     * commented out. It is the manual flavour which I wanted to keep to allow
     * people to understand what's going on.
     *
     * The one shipped is way more elegant (in my opinion) than the manual
     * implementation. It starts from the observation that a 1d interpolation
     * from a mesh onto a subdivided mesh with a subdivision factor by three
     * is very simplistic matrix.
     *
     * \f[
      \begin{array}{cccccccc}
    1 & 0 & 0 & 0 \\
    1 & 0 & 0 & 0 \\
    1 & 0 & 0 & 0 \\
    0 & 1 & 0 & 0 \\
    0 & 1 & 0 & 0 \\
    0 & 1 & 0 & 0 \\
    0 & 0 & 1 & 0 \\
    0 & 0 & 1 & 0 \\
    0 & 0 & 1 & 0 \\
    0 & 0 & 0 & 1 \\
    0 & 0 & 0 & 1 \\
    0 & 0 & 0 & 1
      \end{array}
     \f]
     *
     * Here, I use four-partitioning.
     *
     * ## Formatting of documentation (doxygen)
     *
     * Doxygen nowadays supports markup syntax. Unfortunately, this documentation
     * would be indented more than five spaces which makes some of the write-up
     * preformatted in markup. So the doxygen parser completely messes up things.
     *
     * For this reason, the documentation here is not properly indented.
     */
      tarch::la::DynamicMatrix*  createPiecewiseConstantInterpolationMatrix(int numberOfDoFsPerAxisInPatch, int normal, int overlap);

      /**
       * This is a volumetric version of the interpolation.
       *
       * It is close to trivial, as we can employ internal::projectCells_AoS()
       * to identify pairs of coarse-fine volumes that overlap and then we
       * simply copy over results.
       */
      tarch::la::DynamicMatrix*  createPiecewiseConstantInterpolationMatrix(int numberOfDoFsPerAxisInPatch);

      /**
       * This routine accepts a @f$ 3k \times k @f$ matrix as input. This is the 1d
       * matrix. It clearly maps a sequence of k finite volumes onto a new sequence
       * of 3k volumes.
       *
       * We now exploit the fact any interpolation routine is basically a rotation or
       * mirroring of this operator, while we have to take into account that we need
       * two times k entries as we always store left and right halo overlaps.
       */
      tarch::la::DynamicMatrix*  createInterpolationMatrixFrom1dTemplateByInsertingZeroColsAndRows(
        const tarch::la::DynamicMatrix&  P1d,
        int numberOfDoFsPerAxisInPatch,
        int normal
      );

      /**
       * This is an extension of createInterpolationMatrixFrom1dTemplateByInsertingZeroColsAndRows().
       * In the setup as sketched below
       *
       * @image html Interpolation_createLinearInterpolationMatrix.png
       *
       * we have two options to initialise bright green values: We can either use the bright red
       * volumes only, or we can interpolate between the bright red ones and the dark red ones.
       * This operation does the latter.
       */
      tarch::la::DynamicMatrix*  createInterpolationMatrixFrom1dTemplateByLinearInterpolationAlongNormal(
        const tarch::la::DynamicMatrix&  P1d,
        int numberOfDoFsPerAxisInPatch,
        int normal
      );

      /**
       * This routine assumes that you have the whole patch data of hte left and right
       * adjacent patch at hands.
       *
       * We identify the overlap with the fine halo layer and copy values over from these
       * two arrays onto the face data.
       */
      void projectInterpolatedFineCellsOnHaloLayer_AoS(
        const peano4::datamanagement::FaceMarker& marker,
        int                                       numberOfDoFsPerAxisInPatch,
        int                                       overlap,
        int                                       unknowns,
        const double* __restrict__                fineGridCellValuesLeft,
        const double* __restrict__                fineGridCellValuesRight,
        double*                                   fineGridFaceValues
      );

      /**
       * @see Other interpolateCell_AoS_piecewise_constant() variant which delegates to
       *   this one which is also used by the face interpolation.
       */
      void interpolateCell_AoS_piecewise_constant(
        const tarch::la::Vector<Dimensions,int>&  relativePositionWithinFatherCell,
        int                                       numberOfDoFsPerAxisInPatch,
        int                                       unknowns,
        const double* __restrict__                fineGridValues,
        double*                                   coarseGridValues
      );

      void interpolateCell_AoS_linear(
        const tarch::la::Vector<Dimensions,int>&  relativePositionWithinFatherCell,
        int                                       numberOfDoFsPerAxisInPatch,
        int                                       unknowns,
        const double* __restrict__                fineGridValues,
        double*                                   coarseGridValues,
        bool                                      extrapolateLinearly
      );

      /**
       * @see FaceInterpolationMap
       */
      struct FaceInterpolationOperatorKey {
        int numberOfDoFsPerAxisInPatch;
        int overlap;
        int normal;

        inline FaceInterpolationOperatorKey(
          int numberOfDoFsPerAxisInPatch_,
          int overlap_,
          int normal_
        ):
          numberOfDoFsPerAxisInPatch(numberOfDoFsPerAxisInPatch_),
          overlap(overlap_),
          normal(normal_) {}

        inline bool operator < ( const FaceInterpolationOperatorKey& otherKey ) const {
          return (this->numberOfDoFsPerAxisInPatch < otherKey.numberOfDoFsPerAxisInPatch)
              or (this->numberOfDoFsPerAxisInPatch == otherKey.numberOfDoFsPerAxisInPatch and this->overlap < otherKey.overlap)
              or (this->numberOfDoFsPerAxisInPatch == otherKey.numberOfDoFsPerAxisInPatch and this->overlap == otherKey.overlap and this->normal < otherKey.normal);
        }
      };

      /**
       * @see CellInterpolationMap
       */
      struct CellInterpolationOperatorKey {
        int numberOfDoFsPerAxisInPatch;

        inline CellInterpolationOperatorKey(int numberOfDoFsPerAxisInPatch_):
          numberOfDoFsPerAxisInPatch(numberOfDoFsPerAxisInPatch_) {}

        inline bool operator < ( const CellInterpolationOperatorKey& otherKey ) const {
          return this->numberOfDoFsPerAxisInPatch < otherKey.numberOfDoFsPerAxisInPatch;
        }
      };

      typedef std::map< FaceInterpolationOperatorKey, tarch::la::DynamicMatrix* >  FaceInterpolationMap;

      /**
       * A cell interpolation is a huge matrix. If a patch consists of @f$ p @f$ entries
       * per coordinate axis, then the matrix has the dimensions @f$ p^d \cdot 3^d \times p^d @f$.
       * Its input is always the @f$ p^d @f$ vector of the coarse mesh. The output is typically
       * a @f$ p^d @f$ vector as well, but there are @f$ 3^d @f$ of these guys.
       *
       * We store one huge interpolation matrix and then pick the right band out of it.
       */
      typedef std::map< CellInterpolationOperatorKey, tarch::la::DynamicMatrix* >  CellInterpolationMap;
    }
  }
}
