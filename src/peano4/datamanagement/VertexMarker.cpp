// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VertexMarker.h"

#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/Loop.h"

std::ostream& operator<<(std::ostream& out, const peano4::datamanagement::VertexMarker& marker) {
  out << marker.toString();
  return out;
}

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

peano4::datamanagement::VertexMarker::VertexMarker(const peano4::grid::GridTraversalEvent& event, int select):
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

bool peano4::datamanagement::VertexMarker::isParentVertexLocal(int i) const {
  assertion(i >= 0);
  assertion(i < TWO_POWER_D);
  // assertion(_isParentVertexLocal[i], toString()); // TODO: Is this needed?
  return _isParentVertexLocal[i];
}

bool peano4::datamanagement::VertexMarker::coincidesWithCoarseGridVertex() const {
  bool result = true;
  for (int d = 0; d < DIMENSIONS; d++) {
    result &= getRelativePositionWithinFatherCell()(d) != 1;
    result &= getRelativePositionWithinFatherCell()(d) != 2;
  }
  return result;
}

tarch::la::Vector<DIMENSIONS, double> peano4::datamanagement::VertexMarker::getX(int i) const {
  tarch::la::Vector<DIMENSIONS, double> result(_cellCentre);
  std::bitset<DIMENSIONS>               myset(i);
  for (int d = 0; d < DIMENSIONS; d++) {
    result(d) -= 0.5 * _h(d);
    result(d) += static_cast<double>(myset[d]) * _h(d);
  }
  return result;
}

tarch::la::Vector<DIMENSIONS, double> peano4::datamanagement::VertexMarker::getX() const { return getX(_select); }

std::string peano4::datamanagement::VertexMarker::toString() const {
  std::ostringstream msg;
  msg
    << "(cell-centre=" << _cellCentre << ",cell-h=" << _h << ",local=" << _isLocal << ",hanging=" << _isHanging
    << ",select=" << _select << ",has-been-refined=" << _hasBeenRefined << ",will-be-refined=" << _willBeRefined
    << ",is-parent-vertex-local=" << _isParentVertexLocal << ",is-parent-cell-local=" << _isParentCellLocal
    << ",rel-pos-within-father=" << relativePositionWithinFatherCell
    << ",is-adjacent-cell-local=" << _isAdjacentCellLocal << ",x(selected)=" << getX() << ")";
  return msg.str();
}

peano4::datamanagement::VertexMarker& peano4::datamanagement::VertexMarker::select(int vertexNumber) {
  assertion(vertexNumber >= 0, vertexNumber, toString());
  assertion(vertexNumber < TWO_POWER_D, vertexNumber, toString());
  _select = vertexNumber;
  return *this;
}

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

bool peano4::datamanagement::VertexMarker::isAdjacentToParallelDomainBoundary(int i) const {
  assertion(not _isAdjacentToParallelDomainBoundary[i], toString());
  return _isAdjacentToParallelDomainBoundary[i];
}

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

std::bitset<TWO_POWER_D> peano4::datamanagement::VertexMarker::areAdjacentCellsLocal() const {
  std::bitset<TWO_POWER_D> result = 0;

  std::bitset<DIMENSIONS>            selectedVertexAsBitset = _select;
  tarch::la::Vector<DIMENSIONS, int> selectedVertex(selectedVertexAsBitset);
  dfor2(j) {
    tarch::la::Vector<DIMENSIONS, int> entryIn3x3Patch = selectedVertex + j;
    result[jScalar] = _isAdjacentCellLocal[peano4::dLinearised(entryIn3x3Patch, 3)];
  }
  enddforx

    return result;
}
