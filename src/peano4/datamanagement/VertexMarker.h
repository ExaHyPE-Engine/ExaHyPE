// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"


namespace peano4 {
  namespace datamanagement {
    struct VertexMarker;

    /**
     * Reconstruct parent vertex position
     *
     * Whenever we run into a vertex event, this event is triggered from a
     * cell's point of view. You can find out from the VertexMarker where
     * within this cell the vertex is for which you got a vertex event.
     * Likewisely, you can also reconstruct the spatial arrangement of
     * this cell's parent cell. It is not a core vertex marker routine,
     * which is frequently used, so we outsource it into a separate function
     * of its own. At the moment, we are only aware of some debugging, tracing
     * facilities which need this function.
     *
     * @param parentVertexPositionWithinParentCell follows Peano's
     *   dimension-generic lexicographic enumeration. You can also pass in an
     *   integer, which will automatically be packed into a bitset then.
     */
    tarch::la::Vector<Dimensions, double> reconstructXOfParentVertex(
      const VertexMarker&             marker,
      const std::bitset<Dimensions>&  parentVertexPositionWithinParentCell
    );
  } // namespace datamanagement
} // namespace peano4


namespace toolbox {
  namespace particles {
    namespace tests {
      /**
       * Forward declarations for test purposes
       */
      class MultiscaleTransitionsTest;
      class TestHelpers;
    } // namespace tests
  }   // namespace particles
} // namespace toolbox


std::ostream& operator<<(std::ostream& out, const peano4::datamanagement::VertexMarker& marker);


/**
 * Vertex marker to provide information about selected vertex
 *
 * A vertex marker is handed into each and every observer/action set routine
 * which is vertex-based. That is routines such as touchVertexFirstTime().
 * The marker provides information about the underlying vertex.
 *
 * Notably, we can provide
 *
 * - geometric information such as the position of a vertex or the size of
 *   the adjacent cells.
 * - status information such as whether a vertex is hanging.
 * - grid layout information such as the vertex's position within the
 *   parent cell.
 *
 * ## Rationale
 *
 * Peano's events (observer and action set routines) are internally all
 * implemented on a per cell basis. So we invoke touchVertexFirstTime()
 * from the cell when the first adjacent cell of a vertex is entered.
 * Therefore, we know the selected vertex.
 *
 * Peano runs through the mesh with a tree automaton. A lot of information
 * about vertices thus is recomputed on-the-fly all the time, as it is easy
 * to derive it from the context. We know, for example, from the state of the
 * traversal automaton if a vertex is hanging or not or which position in
 * space it has. Therefore, such information is not stored within the vertex
 * (unless for debugging purposes). Not only would the information be redundant,
 * it would also be very inefficient: If you work with k data records per
 * vertex, each one would hold the same information about the underlying
 * vertex's position. So it makes more sense to encode such information with
 * a separate object.
 *
 * @see FaceMarker for the analogous information for faces.
 */
struct peano4::datamanagement::VertexMarker {
private:
  /**
   * For unit tests
   */
  friend class toolbox::particles::tests::MultiscaleTransitionsTest;
  friend class toolbox::particles::tests::TestHelpers;

  /**
   * Centre of the underlying cell
   */
  tarch::la::Vector<Dimensions, double> _cellCentre;

  /**
   * Size of the underlying cell
   */
  tarch::la::Vector<Dimensions, double> _h;

  int _select;

  [[clang::pack]] std::bitset<TwoPowerD>    _hasBeenRefined;
  [[clang::pack]] std::bitset<TwoPowerD>    _willBeRefined;
  [[clang::pack]] std::bitset<TwoPowerD>    _isLocal;
  [[clang::pack]] std::bitset<TwoPowerD>    _isHanging;
  [[clang::pack]] std::bitset<TwoPowerD>    _isParentVertexLocal;
  [[clang::pack]] std::bitset<ThreePowerD>  _isAdjacentCellLocal;
  [[clang::pack]] std::bitset<TwoPowerD>    _isParentOfSubtree;

  bool _isParentCellLocal;

  bool isParentLocal() const;

  std::bitset<TwoPowerD>  _isAdjacentToParallelDomainBoundary;

  /**
   * Entries from (0,1,2). (0,0) or (0,0,0) is the left, bottom cell.
   */
  tarch::la::Vector<Dimensions, int> _relativePositionOfCellWithinFatherCell;

  /**
   * Some unit tests prefer to construct a marker manually, so I offer a
   * dummy default constructor. The normal code should and does not use a
   * default constructor.
   */
  VertexMarker() = default;

public:
  VertexMarker(const ::peano4::grid::GridTraversalEvent& event, int select = 0);

  /**
   * Picks a vertex within a cell. After that, the routine returns.
   *
   * This routine is only called by Peano's internal observer, i.e. the
   * class constructing VertexMarker objects. Users never call this routine.
   */
  VertexMarker& select(int number);

  /**
   * Information about selected vertex
   *
   * Even though you are given a vertex marker for a vertex routine such as
   * touchVertexFirstTime(), Peano behind the scenes invokes this routine
   * from a cell. That is, it enters a cell, then runs over all @f$ 2^d @f$
   * adjacent vertex and puzzles out for each vertex if it has to call
   * touchVertexFirstTime() or not.
   *
   * getSelectedVertexNumber() tells you which logical number a vertex has
   * within the underlying cell behind the scenes.
   *
   * @image html VertexEnumerator.png
   *
   * The routine is barely used
   * by user codes. Some technical helper routines use the selected number
   * for some arithmetics, but the code indeed might be cleaner if we did
   * hide this routine from the user altogether and make the routine
   * explicitly available only to friends.
   */
  int getSelectedVertexNumber() const;

  /**
   * We do enumerate the vertices in a lexicographic way, i.e. we start with the
   * bottom left vertex. Then we run along the first Cartesian axis, then the
   * second, and so forth. This yields a z-pattern for the enumeration.
   *
   * @return Position of ith vertex
   */
  tarch::la::Vector<Dimensions, double> x(int i) const;

  /**
   * @return Position in space of currently selected vertex.
   */
  tarch::la::Vector<Dimensions, double> x() const;

  bool isParentOfSubtree(int i) const;
  bool isParentOfSubtree() const;

  tarch::la::Vector<Dimensions, double> h() const;

  std::string toString() const;

  /**
   * A vertex is refined iff it is surrounded by @f$ 2^d @f$ cells which are
   * refined.
   */
  bool hasBeenRefined() const;
  bool hasBeenRefined(int i) const;

  bool willBeRefined() const;
  bool willBeRefined(int i) const;

  /**
   * @see isLocal(int) for currently selected/represented vertex
   */
  bool isLocal() const;

  /**
   * Is a vertex local, i.e. not adjacent to any boundary or remote
   */
  bool isLocal(int i) const;

  bool isHanging() const;

  bool isHanging(int i) const;

  /**
   * Is currently selected/analysed vertex adjacent to parallel domain boundary
   */
  bool isAdjacentToParallelDomainBoundary() const;

  /**
   * Is one vertex of cell adjacent to parallel domain boundary
   *
   * Peano runs through the mesh cell by cell. Within a cell, you can ask if
   * the ith vertex actually is adjacent to a parallel domain boundary.
   */
  bool isAdjacentToParallelDomainBoundary(int i) const;

  /**
   * Whenever we touch a vertex the first or the last time, we touch
   * it from a cell: If we touch a vertex the first time, the traversal
   * is just about to enter a cell and sees that one of its vertices is
   * not loaded yet. When we issue a touchVertexLastTime() event, the
   * traversal is just about to leave a cell.
   *
   * So there's a cell state associated with any vertex event, and,
   * therefore, we know if the corresponding cell is local and whether
   * its parent cell is local.
   *
   * @see isParentVertexLocal()
   */
  bool isParentCellLocal() const;

  /**
   * Is the parent vertex local
   *
   * Is a parent vertex local, i.e. not adjacent to any boundary or
   * remote. Peano traverses the tree in a strict octant-wise
   * (multiscale-wise) order. That is, whenever you receive a
   * touchVertexFirstTime() or touchVertexLastTime() event, the Peano
   * core has actually entered a cell and recognised "whoops, I haven't
   * used that vertex yet" or "this will be the last time this vertex
   * is available". See the @ref peano_action_sets "generic event description" on
   * an overview of all of these events.
   *
   * @image html VertexMarker_isParentVertexLocal.png
   *
   * As a logical consequence, there are always @f$ 2^d @f$ parent
   * vertices when you encounter a vertex for the first or last time.
   * But you never know which parent cell it will be. The code could
   * have stepped down from A into a or from B into b. So the parent
   * vertices could be the green or the blue ones.
   *
   * This routine allows you to find out if these vertices are local.
   * It will always return if isParentCellLocal() does not hold.
   */
  bool isParentVertexLocal(int i) const;


  /**
   * Relative position within parent cell
   *
   * Return relative position within father cell. The result is from
   * (0,1,2,3)^d and denotes the vertex position within a 3x3 or 3x3x3
   * respectively mesh.
   *
   * You can reconstruct the left bottom vertex of the coarser cell through
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * marker.x() - tarch::la::multiplyComponents(
   *   tarch::la::convertScalar<double>(marker.getRelativePositionWithinFatherCell()),
   *   marker.h()
   * )
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * If you want to know a particular position of a coarse grid vertex
   * identified through the bitset target, you can do through
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * marker.x()
   * -
   * tarch::la::multiplyComponents(
   *   tarch::la::convertScalar<double>(marker.getRelativePositionWithinFatherCell()),
   *   marker.h()
   * )
   * +
   * tarch::la::multiplyComponents(
   *   tarch::la::Vector<Dimensions,double>(target),
   *   3.0*marker.h()
   * )
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * @see coincidesWithCoarseGridVertex()
   */
  tarch::la::Vector<Dimensions, int> getRelativePositionWithinFatherCell() const;
  tarch::la::Vector<Dimensions, int> getRelativePositionWithinFatherCell(int i) const;

  /**
   * Does vertex spatially coincide with coarser level's vertex
   *
   * The vertex (0,0,0) coincides with a coarse grid vertex, the (3,0,0) does
   * so too, and so forth.
   *
   * ## Frequent use case
   *
   * If you want to program some simply injection, it helps to identify vertices
   * which coincide spatially with vertices on the next coarser level. A classic
   * are codes like
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~
   *       if ( marker.coincidesWithCoarseGridVertex() ) {
   *         tarch::la::Vector<Dimensions,int> parent = marker.getRelativePositionWithinFatherCell() / 3;
   *         coarseGridVerticesX( peano4::utils::dLinearised(parent,2) ).set...
   *       }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * within touchVertexLastTime. This snippet looks if two vertices coincide
   * spatially. If so, it computes the parent's relative position within the
   * parent cell which is simply a division by 3. We then linearise this vector
   * index and thus have access to the parent.
   */
  bool coincidesWithCoarseGridVertex() const;


  /**
   * Will return true if x is contained within the adjacent cells. You can use
   * shrink those cells via the additional parameter if you want.
   *
   * The geometric operation is defined over the closed interval, i.e. we
   * look into the adjacent cubes to the vertex and their faces still count as
   * "inside". If x is exactly on the face of a cube adjacent to the vertex,
   * we will get back a true. Indeed, the cubes are even slightly bigger due
   * to machine precision.
   */
  bool isContainedInAdjacentCells(
    const tarch::la::Vector<Dimensions, double>& x,
    double                                       scaleAdjacentCells = 1.0,
    double                                       tolerance          = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  ) const;


  /**
   * Each vertex is surrounded by @f$ 2^d @f$ cells. In reality,
   * it might be fewer of them, as we might have to deal with a hanging vertex,
   * but logically we can always assume that there were @f$ 2^d @f$.
   * Let's assume that they are grouped lexicographically around the vertex,
   * i.e. we have
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * 2 | 3
   * --+--
   * 0 | 1
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * cells centred around a central vertex denoted by the plus in this fancy
   * ASCII art. If x falls into the cell 1, this routine would return (1,0).
   * If it falls into cell 3, it would return (1,1).
   *
   */
  tarch::la::Vector<Dimensions, int> getRelativeAdjacentCell(const tarch::la::Vector<Dimensions, double>& x) const;

  /**
   * Return centre of cell from which this vertex marker is constructed
   *
   * Peano runs through the mesh element-wisely. That is, each
   * touchVertexFirstTime() and touchVertexLastTime() is called from a cell.
   * It looks at its @f$ 2^d @f$ adjacent vertices and then invokes the vertex
   * events for these vertices if necessary. This routine yields the centre of
   * this cell.
   */
  tarch::la::Vector<Dimensions, double> getInvokingCellCentre() const;

  /**
   * See getInvokingCellCentre(). This routine gives you the centre of its
   * parent cell.
   */
  tarch::la::Vector<Dimensions, double> getInvokingParentCellsCentre() const;

  /**
   * Are adjacent cells of selected vertex local
   *
   * ## Correctness
   *
   * I originally thought I could add the assertion
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *   assertion2( result.all() or isAdjacentToParallelDomainBoundary(), result, toString() );
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * However, this assertion fails at the global domain boundary, where some
   * adjacent cells are not local (they are outside) and the vertices
   * nevertheless are not parallel boundary vertices either.
   */
  std::bitset<TwoPowerD> areAdjacentCellsLocal() const;
};
