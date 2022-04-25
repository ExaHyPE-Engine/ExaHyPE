// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_BLOCKSTRUCTURED_INTERPOLATION_H_
#define _TOOLBOX_BLOCKSTRUCTURED_INTERPOLATION_H_


#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"

#include "tarch/la/DynamicMatrix.h"


#include <map>


namespace toolbox {
  namespace blockstructured {
    /**
     * @page Interpolation Halo layer interpolation
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
     * @f$  u_h = P u_H @f$
     *
     * for $N$ versions of @f$u_h@f$ and @f$u_H@f$. Peano's tarch component has a routine
     * for batched multiplication, where you pipe in a set of @f$u_H@f$ vectors and you
     * get a set of @f$u_h@f$ vectors out.
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
     * ## Caching of inter-grid transfer operators
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
     * fine grid volume values and write them onto teh face data as copies.
     *
     *
     */





    /**
     * Take the coarse grid values and interpolate them onto the fine grid
     *
     * We run over all coarse grid volumes (counter k or coarseGridVolume,
     * respectively). Each coarse grid volume overlaps 1x3x3 or 3x1x3 or ...
     * smaller volumes. For both loops, I use the dfore (exclusive, d-dimensional
     * for).
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


    void interpolateHaloLayer_AoS_linear(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridFaceValues,
      double*                                   coarseGridFaceValues
    );


    void interpolateHaloLayer_AoS_linear(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   coarseGridCellValues,
      double*                                   coarseGridFaceValues,
      double*                                   fineGridFaceValues
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
    void interpolateHaloLayer_AoS_outflow(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    );


    /**
     * This type of signature is used to interpolate halo layers for newly
     * generated cells. So this does not really make sense for outflow, as
     * there's no outflow from fine to coarse resolution if we create a new
     * cell. Thererfore, I do not implement this routine, but simply delegate
     * to piecewise constant interpolation.
     */
    void interpolateHaloLayer_AoS_outflow(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridFaceValues,
      double*                                   coarseGridFaceValues,
      double*                                   coarseGridCellValues
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
       * <h2> Insertion of empty lines/rows </h2>
       *
       * We assume that the result is "symmetric" - not in a linear equation system
       * sense, but in that the columns and row insertions always have to match.
       *
       *
       * @param pattern What shift/replication pattern do you want. 1 means that
       *  each row/column is repeated immediately (subject to a shift of 1).
       *  14 means that each block of 14 entries is repeated. Typically it is
       *  numberOfDoFsPerAxisInPatch^k with k ranging from 0 to Dimensions-1.
       *
       *  Feel free to pass in -1 to switch this feature off. I currently use the
       *  argument -1 for testing only.
       *
       * @see tarch::la::DynamicMatrix::insertColumns()
       */
      tarch::la::DynamicMatrix*  createLinearInterpolationMatrix(int numberOfDoFsPerAxisInPatch, int normal);

      /**
       * This is a volumetric version of the interpolation.
       */
      tarch::la::DynamicMatrix*  createLinearInterpolationMatrix(int numberOfDoFsPerAxisInPatch);

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

      /**
       * @see FaceInterpolationMap
       */
      struct FaceInterpolationOperatorKey {
        int numberOfDoFsPerAxisInPatch;
        int overlap;
        int normal;

        FaceInterpolationOperatorKey(
          int numberOfDoFsPerAxisInPatch_,
          int overlap_,
          int normal_
        ):
          numberOfDoFsPerAxisInPatch(numberOfDoFsPerAxisInPatch_),
          overlap(overlap_),
          normal(normal_) {}

        bool operator<( const FaceInterpolationOperatorKey&  otherKey ) const {
          return this->numberOfDoFsPerAxisInPatch < otherKey.numberOfDoFsPerAxisInPatch
              or this->numberOfDoFsPerAxisInPatch == otherKey.numberOfDoFsPerAxisInPatch and this->overlap < otherKey.overlap
              or this->numberOfDoFsPerAxisInPatch == otherKey.numberOfDoFsPerAxisInPatch and this->overlap == otherKey.overlap and this->normal < otherKey.normal;
        }
      };


      /**
       * @see CellInterpolationMap
       */
      struct CellInterpolationOperatorKey {
        int numberOfDoFsPerAxisInPatch;

        CellInterpolationOperatorKey(
          int numberOfDoFsPerAxisInPatch_
        ):
          numberOfDoFsPerAxisInPatch(numberOfDoFsPerAxisInPatch_) {}

        bool operator<( const CellInterpolationOperatorKey&  otherKey ) const {
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


#endif
