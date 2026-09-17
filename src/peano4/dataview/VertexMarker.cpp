// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VertexMarker.h"

#include "peano4/Loop.h"

static_assert(std::is_standard_layout_v<peano4::dataview::VertexMarker>);
static_assert(std::is_trivially_copyable_v<peano4::dataview::VertexMarker>);

std::ostream& peano4::dataview::operator<<(std::ostream& out, const peano4::dataview::VertexMarker& marker) {
  out << marker.toString();
  return out;
}

/**
 * Returns the position of one vertex of the parent cell of the cell the marker
 * was built from.
 *
 * @param parentVertexPositionWithinParentCell follows Peano's
 *   dimension-generic lexicographic enumeration.
 */
tarch::la::Vector<DIMENSIONS, double> peano4::dataview::reconstructXOfParentVertex(
  const peano4::dataview::VertexMarker& marker,
  const std::bitset<DIMENSIONS>&        parentVertexPositionWithinParentCell
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

peano4::dataview::VertexMarker::VertexMarker(const peano4::grid::GridTraversalEvent& event, const int selectedVertex) :
  _cellCentre(event.getX()),
  _h(event.getH()),
  _select(selectedVertex),
  _hasBeenRefined(event.getHasBeenRefined()),
  _willBeRefined(event.getWillBeRefined()),
  _isLocal(event.getIsVertexLocal()),
  _isHanging(event.getIsVertexHanging()),
  _isParentVertexLocal(event.getIsParentVertexLocal()),
  _isAdjacentCellLocal(event.getIsAdjacentCellLocal()),
  _isParentOfSubtree(event.getIsVertexParentOfSubtree()),
  _isParentCellLocal(event.getIsParentCellLocal()),
  _isAdjacentToParallelDomainBoundary(event.getIsVertexAdjacentToParallelDomainBoundary()),
  _relativePositionWithinFatherCell(event.getRelativePositionToFather()) {
  validateVertexNumber(selectedVertex);
}

std::string peano4::dataview::VertexMarker::toString() const {
  std::ostringstream out;
  out << std::boolalpha;
  out << "VertexMarker{";
  out << "cellCentre=" << ::toString(_cellCentre);
  out << ",h=" << ::toString(_h);
  out << ",isLocal=" << _isLocal;
  out << ",isHanging=" << _isHanging;
  out << ",selectedVertex=" << _select;
  out << ",hasBeenRefined=" << _hasBeenRefined;
  out << ",willBeRefined=" << _willBeRefined;
  out << ",isParentVertexLocal=" << _isParentVertexLocal;
  out << ",isParentCellLocal=" << _isParentCellLocal;
  out << ",relativePositionWithinFatherCell=" << ::toString(_relativePositionWithinFatherCell);
  out << ",isAdjacentCellLocal=" << _isAdjacentCellLocal;
  out << ",x=" << ::toString(getX());
  out << "}";
  return out.str();
}

/**
 * Returns which cells adjacent to the selected vertex are local. A vertex on
 * the global domain boundary has non-local adjacent cells that lie outside the
 * domain, so a vertex with non-local adjacent cells need not be adjacent to a
 * parallel domain boundary.
 */
std::bitset<TWO_POWER_D> peano4::dataview::VertexMarker::areAdjacentCellsLocal() const {
  auto                                     result             = std::bitset<TWO_POWER_D>{};
  const std::bitset<DIMENSIONS>            selectedVertexBits = _select;
  const tarch::la::Vector<DIMENSIONS, int> selectedVertex(selectedVertexBits);
  peano4::dForEachCorner(
    [&](const tarch::la::Vector<DIMENSIONS, int>& adjacentCellOffset, const int adjacentCellNumber) {
      const auto positionInPatch = selectedVertex + adjacentCellOffset;
      result[adjacentCellNumber] = _isAdjacentCellLocal[peano4::dLinearised(positionInPatch, 3)];
    }
  );
  return result;
}
