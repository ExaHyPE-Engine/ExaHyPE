// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VertexMarker.h"

#include "peano4/Loop.h"
#include "peano4/grid/GridTraversalObserver.h"

std::ostream& operator<<(std::ostream& out, const peano4::datamanagement::VertexMarker& marker) {
  out << marker.toString();
  return out;
}

/**
 * Reconstruct parent vertex position
 *
 * A vertex event is always triggered from a cell. The marker tells us where
 * the vertex sits within that cell, and from there we can derive the spatial
 * arrangement of the parent cell. Debugging and tracing facilities are the
 * typical users of this information.
 *
 * @param parentVertexPositionWithinParentCell follows Peano's
 *   dimension-generic lexicographic enumeration. An integer argument is
 *   packed into a bitset automatically.
 */
tarch::la::Vector<DIMENSIONS, double> peano4::datamanagement::reconstructXOfParentVertex(
  const VertexMarker&            marker,
  const std::bitset<DIMENSIONS>& parentVertexPositionWithinParentCell
) {
  return marker.getX()
    - tarch::la::multiplyComponents(
           tarch::la::convertScalar<double>(marker.getRelativePositionWithinFatherCell()),
           marker.getH()
    )
    + tarch::la::multiplyComponents(
           tarch::la::Vector<DIMENSIONS, double>(parentVertexPositionWithinParentCell),
           3.0 * marker.getH()
    );
}

peano4::datamanagement::VertexMarker::VertexMarker(const peano4::grid::GridTraversalEvent& event, int select) :
  _cellCentre(event.getX()),
  _h(event.getH()),
  _select(select) {
  _hasBeenRefined                     = event.getHasBeenRefined();
  _willBeRefined                      = event.getWillBeRefined();
  _isLocal                            = event.getIsVertexLocal();
  _isParentCellLocal                  = event.getIsParentCellLocal();
  relativePositionWithinFatherCell    = event.getRelativePositionToFather();
  _isAdjacentToParallelDomainBoundary = event.getIsVertexAdjacentToParallelDomainBoundary();
  _isAdjacentCellLocal                = event.getIsAdjacentCellLocal();
  _isParentOfSubtree                  = event.getIsVertexParentOfSubtree();
  _isHanging                          = event.getIsVertexHanging();
  _isParentVertexLocal                = event.getIsParentVertexLocal();
}

/**
 * Relative position within parent cell
 *
 * The result is from (0,1,2,3)^d and denotes the vertex position within the
 * 3x3 or 3x3x3 pattern into which the parent cell is refined.
 *
 * The left bottom vertex of the coarser cell follows from
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * marker.getX() - tarch::la::multiplyComponents(
 *   tarch::la::convertScalar<double>(marker.getRelativePositionWithinFatherCell()),
 *   marker.getH()
 * )
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * and the position of the coarse grid vertex identified through the bitset
 * target from
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * marker.getX()
 * -
 * tarch::la::multiplyComponents(
 *   tarch::la::convertScalar<double>(marker.getRelativePositionWithinFatherCell()),
 *   marker.getH()
 * )
 * +
 * tarch::la::multiplyComponents(
 *   tarch::la::Vector<DIMENSIONS,double>(target),
 *   3.0*marker.getH()
 * )
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @see coincidesWithCoarseGridVertex()
 */
tarch::la::Vector<DIMENSIONS, int> peano4::datamanagement::VertexMarker::getRelativePositionWithinFatherCell() const {
  return getRelativePositionWithinFatherCell(_select);
}

tarch::la::Vector<DIMENSIONS, int> peano4::datamanagement::VertexMarker::getRelativePositionWithinFatherCell(
  int i
) const {
  tarch::la::Vector<DIMENSIONS, int> result = relativePositionWithinFatherCell;
  std::bitset<DIMENSIONS>            index  = i;
  for (int d = 0; d < DIMENSIONS; d++) {
    result(d) += index[d] ? 1 : 0;
  }
  return result;
}

/**
 * Is the parent vertex local
 *
 * Peano traverses the tree in a strict octant-wise (multiscale) order, so a
 * touchVertexFirstTime() or touchVertexLastTime() event is always raised from
 * within a cell that the traversal has just entered or is about to leave. See
 * the @ref peano_action_sets "generic event description" for an overview of
 * these events.
 *
 * @image html VertexMarker_isParentVertexLocal.png
 *
 * There are consequently always @f$ 2^d @f$ parent vertices, but which parent
 * cell they belong to is not known: the traversal could have stepped down from
 * A into a or from B into b, so the parent vertices are either the green or the
 * blue ones. This routine tells us whether they are local.
 *
 * @see isParentCellLocal()
 */
bool peano4::datamanagement::VertexMarker::isParentVertexLocal(int i) const {
  assertion(i >= 0);
  assertion(i < TWO_POWER_D);
  return _isParentVertexLocal[i];
}

/**
 * Does vertex spatially coincide with coarser level's vertex
 *
 * The vertex (0,0,0) coincides with a coarse grid vertex, the (3,0,0) does so
 * too, and so forth.
 *
 * ## Frequent use case
 *
 * A simple injection needs to identify the vertices which coincide spatially
 * with vertices on the next coarser level:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
 *       if ( marker.coincidesWithCoarseGridVertex() ) {
 *         tarch::la::Vector<DIMENSIONS,int> parent = marker.getRelativePositionWithinFatherCell() / 3;
 *         coarseGridVerticesX( peano4::dLinearised(parent,2) ).set...
 *       }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Within touchVertexLastTime, this snippet divides the relative position by 3
 * to obtain the parent's position within its own parent cell, and linearises
 * that vector index to access the parent.
 */
bool peano4::datamanagement::VertexMarker::coincidesWithCoarseGridVertex() const {
  bool result = true;
  for (int d = 0; d < DIMENSIONS; d++) {
    result &= getRelativePositionWithinFatherCell()(d) != 1;
    result &= getRelativePositionWithinFatherCell()(d) != 2;
  }
  return result;
}

/**
 * The vertices are enumerated lexicographically: we start with the bottom left
 * vertex, then run along the first Cartesian axis, then the second, and so
 * forth. This yields a z-pattern for the enumeration.
 */
tarch::la::Vector<DIMENSIONS, double> peano4::datamanagement::VertexMarker::getX(int i) const {
  tarch::la::Vector<DIMENSIONS, double> result(_cellCentre);
  const std::bitset<DIMENSIONS>         myset(i);
  for (int d = 0; d < DIMENSIONS; d++) {
    result(d) -= 0.5 * _h(d);
    // Testing the bit avoids converting the bitset proxy into a floating-point factor,
    // which is unreliable once a compiler preserves exception semantics.
    if (myset[d]) {
      result(d) += _h(d);
    }
  }
  return result;
}

tarch::la::Vector<DIMENSIONS, double> peano4::datamanagement::VertexMarker::getX() const { return getX(_select); }

std::string peano4::datamanagement::VertexMarker::toString() const {
  std::ostringstream msg;
  msg << "(cell-centre=" << _cellCentre << ",cell-h=" << _h << ",local=" << _isLocal << ",hanging=" << _isHanging
      << ",select=" << _select << ",has-been-refined=" << _hasBeenRefined << ",will-be-refined=" << _willBeRefined
      << ",is-parent-vertex-local=" << _isParentVertexLocal << ",is-parent-cell-local=" << _isParentCellLocal
      << ",rel-pos-within-father=" << relativePositionWithinFatherCell
      << ",is-adjacent-cell-local=" << _isAdjacentCellLocal << ",x(selected)=" << getX() << ")";
  return msg.str();
}

/**
 * Peano's internal observer, i.e. the class which constructs VertexMarker
 * objects, is the only caller of this routine.
 */
peano4::datamanagement::VertexMarker& peano4::datamanagement::VertexMarker::select(int vertexNumber) {
  assertion(vertexNumber >= 0, vertexNumber, toString());
  assertion(vertexNumber < TWO_POWER_D, vertexNumber, toString());
  _select = vertexNumber;
  return *this;
}

/**
 * Is x contained within the adjacent cells
 *
 * The adjacent cells can be shrunk or grown through scaleAdjacentCells. The
 * geometric test is defined over the closed interval: a point exactly on a
 * face of one of the adjacent cubes still counts as inside, and the tolerance
 * widens the cubes slightly further to absorb machine precision.
 */
bool peano4::datamanagement::VertexMarker::isContainedInAdjacentCells(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  double                                       scaleAdjacentCells,
  double                                       tolerance
) const {
  const tarch::la::Vector<DIMENSIONS, double> selectedVertex = getX();
  bool                                        result         = true;
  for (int d = 0; d < DIMENSIONS; d++) {
    const double distance = std::abs(x(d) - selectedVertex(d));
    const double reach    = scaleAdjacentCells * _h(d);
    result &= distance <= reach + tolerance;
  }
  return result;
}

/**
 * One-dimensional version of isContainedInAdjacentCells()
 *
 * @param dim Search direction with 0 <= dim < DIMENSIONS.
 */
bool peano4::datamanagement::VertexMarker::isContainedInAdjacentCellsByDimension(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const int                                    dim,
  double                                       scaleAdjacentCells,
  double                                       tolerance
) const {
  const tarch::la::Vector<DIMENSIONS, double> selectedVertex = getX();
  const double                                distance       = std::abs(x(dim) - selectedVertex(dim));
  const double                                reach          = scaleAdjacentCells * _h(dim);
  return distance <= reach + tolerance;
}

/**
 * Which adjacent cell holds x
 *
 * Each vertex is surrounded by @f$ 2^d @f$ cells. There may be fewer of them
 * around a hanging vertex, but logically we can always assume @f$ 2^d @f$.
 * They are grouped lexicographically around the vertex:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 2 | 3
 * --+--
 * 0 | 1
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * The plus marks the vertex. If x falls into cell 1, the routine returns
 * (1,0). If it falls into cell 3, it returns (1,1).
 */
tarch::la::Vector<DIMENSIONS, int> peano4::datamanagement::VertexMarker::getRelativeAdjacentCell(
  const tarch::la::Vector<DIMENSIONS, double>& x
) const {
  tarch::la::Vector<DIMENSIONS, int> result;
  for (int d = 0; d < DIMENSIONS; d++) {
    result(d) = x(d) <= (this->getX())(d) ? 0 : 1;
  }
  return result;
}

bool peano4::datamanagement::VertexMarker::isAdjacentToParallelDomainBoundary() const {
  return isAdjacentToParallelDomainBoundary(_select);
}

/**
 * Peano runs through the mesh cell by cell, so the marker can report per
 * vertex of the cell whether it is adjacent to a parallel domain boundary.
 */
bool peano4::datamanagement::VertexMarker::isAdjacentToParallelDomainBoundary(int i) const {
  assertion(i >= 0);
  assertion(i < TWO_POWER_D);
  return _isAdjacentToParallelDomainBoundary[i];
}

/**
 * Centre of the cell from which this vertex marker is constructed
 *
 * Peano runs through the mesh element-wise, so each touchVertexFirstTime() and
 * touchVertexLastTime() is issued from a cell which looks at its @f$ 2^d @f$
 * adjacent vertices and raises the vertex events where necessary.
 */
tarch::la::Vector<DIMENSIONS, double> peano4::datamanagement::VertexMarker::getInvokingCellCentre() const {
  return _cellCentre;
}

tarch::la::Vector<DIMENSIONS, double> peano4::datamanagement::VertexMarker::getInvokingParentCellsCentre() const {
  tarch::la::Vector<DIMENSIONS, double> result = _cellCentre;
  for (int d = 0; d < DIMENSIONS; d++) {
    assertion(relativePositionWithinFatherCell(d) >= 0, toString());
    assertion(relativePositionWithinFatherCell(d) <= 2, toString());
    if (relativePositionWithinFatherCell(d) == 0) {
      result(d) += _h(d);
    }
    if (relativePositionWithinFatherCell(d) == 2) {
      result(d) -= _h(d);
    }
  }
  return result;
}

/**
 * Are adjacent cells of selected vertex local
 *
 * A vertex can have non-local adjacent cells without being adjacent to a
 * parallel domain boundary: at the global domain boundary the missing cells
 * lie outside the domain.
 */
std::bitset<TWO_POWER_D> peano4::datamanagement::VertexMarker::areAdjacentCellsLocal() const {
  std::bitset<TWO_POWER_D> result = 0;

  std::bitset<DIMENSIONS>            selectedVertexAsBitset = _select;
  tarch::la::Vector<DIMENSIONS, int> selectedVertex(selectedVertexAsBitset);
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& j, int jScalar) {
    tarch::la::Vector<DIMENSIONS, int> entryIn3x3Patch = selectedVertex + j;
    result[jScalar]                                    = _isAdjacentCellLocal[peano4::dLinearised(entryIn3x3Patch, 3)];
  });

  return result;
}
