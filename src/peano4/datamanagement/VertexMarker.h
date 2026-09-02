// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  namespace datamanagement {
    struct VertexMarker;

    /**
     * Reconstruct the position of a vertex of the parent cell.
     */
    tarch::la::Vector<DIMENSIONS, double> reconstructXOfParentVertex(
      const VertexMarker&            marker,
      const std::bitset<DIMENSIONS>& parentVertexPositionWithinParentCell
    );
  } // namespace datamanagement
} // namespace peano4

namespace peano4::particles::tests {
  /**
   * Forward declarations for test purposes
   */
  class MultiscaleTransitionsTest;
  class TestHelpers;
} // namespace peano4::particles::tests

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
 * (unless for debugging purposes): with k data records per vertex, every
 * single record would carry a copy of the same position. A separate marker
 * object holds it once instead.
 *
 * @see FaceMarker for the analogous information for faces.
 */
struct peano4::datamanagement::VertexMarker {
private:
  /**
   * For unit tests
   */
  friend class peano4::particles::tests::MultiscaleTransitionsTest;
  friend class peano4::particles::tests::TestHelpers;

  /**
   * Centre of the underlying cell
   */
  tarch::la::Vector<DIMENSIONS, double> _cellCentre;

  /**
   * Size of the underlying cell
   */
  tarch::la::Vector<DIMENSIONS, double> _h;

  int _select;

  std::bitset<TWO_POWER_D>   _hasBeenRefined;
  std::bitset<TWO_POWER_D>   _willBeRefined;
  std::bitset<TWO_POWER_D>   _isLocal;
  std::bitset<TWO_POWER_D>   _isHanging;
  std::bitset<TWO_POWER_D>   _isParentVertexLocal;
  std::bitset<THREE_POWER_D> _isAdjacentCellLocal;
  std::bitset<TWO_POWER_D>   _isParentOfSubtree;

  bool _isParentCellLocal;

  std::bitset<TWO_POWER_D> _isAdjacentToParallelDomainBoundary;

  /**
   * Entries from (0,1,2). (0,0) or (0,0,0) is the left, bottom cell.
   */
  tarch::la::Vector<DIMENSIONS, int> relativePositionWithinFatherCell;

  /**
   * Unit tests assemble a marker manually and hence need a default
   * constructor. Production code always constructs from a traversal event.
   */
  VertexMarker() = default;

public:
  VertexMarker(const peano4::grid::GridTraversalEvent& event, int select = 0);

  /**
   * Pick the vertex within the cell to which this marker refers.
   */
  VertexMarker& select(int vertexNumber);

  /**
   * Logical number of the selected vertex within the underlying cell
   *
   * A vertex event such as touchVertexFirstTime() is issued from a cell: the
   * traversal enters a cell, runs over all @f$ 2^d @f$ adjacent vertices and
   * decides per vertex whether the event is due. This routine yields the
   * number the selected vertex carries within that cell.
   *
   * @image html VertexEnumerator.png
   */
  int getSelectedVertexNumber() const { return _select; }

  /**
   * @return Position of ith vertex
   */
  tarch::la::Vector<DIMENSIONS, double> getX(int i) const;

  /**
   * @return Position in space of currently selected vertex.
   */
  tarch::la::Vector<DIMENSIONS, double> getX() const;

  inline bool isParentOfSubtree(int i) const { return _isParentOfSubtree[i]; }
  inline bool isParentOfSubtree() const { return isParentOfSubtree(_select); }

  inline tarch::la::Vector<DIMENSIONS, double> getH() const { return _h; }

  std::string toString() const;

  /**
   * A vertex is refined once every adjacent cell that exists is refined. On the
   * domain wall that is fewer than @f$ 2^d @f$ cells, because the slots outside
   * the domain hold none.
   */
  inline bool hasBeenRefined() const { return hasBeenRefined(_select); }
  inline bool hasBeenRefined(int i) const { return _hasBeenRefined[i]; }

  inline bool willBeRefined() const { return willBeRefined(_select); }
  inline bool willBeRefined(int i) const { return _willBeRefined[i]; }

  /**
   * @see isLocal(int) for currently selected/represented vertex
   */
  inline bool isLocal() const { return isLocal(_select); }

  /**
   * Is a vertex local, i.e. not adjacent to any boundary or remote
   */
  inline bool isLocal(int i) const { return _isLocal[i]; }

  inline bool isHanging() const { return isHanging(_select); }

  inline bool isHanging(int i) const { return _isHanging[i]; }

  /**
   * Is currently selected/analysed vertex adjacent to parallel domain boundary
   */
  bool isAdjacentToParallelDomainBoundary() const;

  /**
   * Is the ith vertex of the cell adjacent to a parallel domain boundary
   */
  bool isAdjacentToParallelDomainBoundary(int i) const;

  /**
   * Is the parent of the cell that issued this vertex event local
   *
   * Every vertex event carries the state of the cell it is issued from, so
   * the locality of that cell's parent is known as well.
   *
   * @see isParentVertexLocal()
   */
  inline bool isParentCellLocal() const { return _isParentCellLocal; }

  /**
   * Is the ith parent vertex local, i.e. neither remote nor outside.
   */
  bool isParentVertexLocal(int i) const;

  /**
   * Position of the vertex within the 3x3 or 3x3x3 pattern of the parent cell.
   */
  tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell() const;
  tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell(int i) const;

  /**
   * Does the vertex spatially coincide with a vertex of the coarser level.
   */
  bool coincidesWithCoarseGridVertex() const;

  /**
   * Is x contained within the cells adjacent to the selected vertex.
   */
  bool isContainedInAdjacentCells(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       scaleAdjacentCells = 1.0,
    double                                       tolerance          = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  ) const;

  /**
   * One-dimensional version of isContainedInAdjacentCells().
   */
  bool isContainedInAdjacentCellsByDimension(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const int                                    dim,
    double                                       scaleAdjacentCells = 1.0,
    double                                       tolerance          = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  ) const;

  /**
   * Which of the cells around the selected vertex holds x.
   */
  tarch::la::Vector<DIMENSIONS, int> getRelativeAdjacentCell(const tarch::la::Vector<DIMENSIONS, double>& x) const;

  /**
   * Centre of the cell from which this vertex marker is constructed.
   */
  tarch::la::Vector<DIMENSIONS, double> getInvokingCellCentre() const;

  /**
   * Centre of the parent of the cell from which this marker is constructed.
   */
  tarch::la::Vector<DIMENSIONS, double> getInvokingParentCellsCentre() const;

  /**
   * Are the cells adjacent to the selected vertex local.
   */
  std::bitset<TWO_POWER_D> areAdjacentCellsLocal() const;
};
