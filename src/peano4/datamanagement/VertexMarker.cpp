#include "VertexMarker.h"
#include "peano4/grid/TraversalObserver.h"


std::ostream& operator<<( std::ostream& out, const peano4::datamanagement::VertexMarker& marker ) {
  out << marker.toString();
  return out;
}


peano4::datamanagement::VertexMarker::VertexMarker(
  const peano4::grid::GridTraversalEvent&   event,
  int                                       select
):
  _cellCentre(event.getX()),
  _h(event.getH()),
  _select(select) {
  _hasBeenRefined                         = event.getHasBeenRefined();
  _willBeRefined                          = event.getWillBeRefined();
  _isLocal                                = event.getIsVertexLocal();
  _isParentCellLocal                      = event.getIsParentCellLocal();
  _relativePositionOfCellWithinFatherCell = event.getRelativePositionToFather();

  _isHanging = 0;
  for (int i=0; i<TwoPowerD; i++) {
    if (
      event.getVertexDataTo(i)==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
    ) {
      int position = event.getVertexDataFrom(i);
      assertion( not _isHanging[position] );
      _isHanging[position] = true;
    }
    if (
      event.getVertexDataFrom(i)==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
    ) {
      int position = event.getVertexDataTo(i);
      assertion( not _isHanging[position] );
      _isHanging[position] = true;
    }
  }
}


tarch::la::Vector<Dimensions,int>  peano4::datamanagement::VertexMarker::getRelativePositionWithinFatherCell() const {
  return getRelativePositionWithinFatherCell(_select);
}


tarch::la::Vector<Dimensions,int>  peano4::datamanagement::VertexMarker::getRelativePositionWithinFatherCell(int i) const {
  tarch::la::Vector<Dimensions,int> result = _relativePositionOfCellWithinFatherCell;
  std::bitset<Dimensions> index = i;
  for (int d=0; d<Dimensions; d++) {
    result(d) += index[d] ? 1 : 0;
  }
  return result;
}


bool peano4::datamanagement::VertexMarker::coincidesWithCoarseGridVertex() const {
  bool result = true;
  for (int d=0; d<Dimensions; d++) {
    result &= getRelativePositionWithinFatherCell()(d) != 1;
    result &= getRelativePositionWithinFatherCell()(d) != 2;
  }
  return result;
}

tarch::la::Vector<Dimensions,double> peano4::datamanagement::VertexMarker::x(int i) const {
  tarch::la::Vector<Dimensions,double> result( _cellCentre );
  std::bitset<Dimensions> myset(i);
  for (int d=0; d<Dimensions; d++) {
    result(d) -= 0.5 * _h(d);
    result(d) += static_cast<double>(myset[d]) * _h(d);
  }
  return result;
}


tarch::la::Vector<Dimensions,double> peano4::datamanagement::VertexMarker::x() const {
  return x(_select);
}


std::string peano4::datamanagement::VertexMarker::toString() const {
  std::ostringstream msg;
  msg << "(" << _cellCentre << "," << _h << ",hanging=" << _isHanging << ",select=" << _select << ")";
  return msg.str();
}


tarch::la::Vector<Dimensions,double>  peano4::datamanagement::VertexMarker::h() const {
  return _h;
}


peano4::datamanagement::VertexMarker& peano4::datamanagement::VertexMarker::select(int vertexNumber) {
  assertion2(vertexNumber>=0,vertexNumber,toString());
  assertion2(vertexNumber<TwoPowerD,vertexNumber,toString());
  _select = vertexNumber;
  return *this;
}


int peano4::datamanagement::VertexMarker::getSelectedVertexNumber() const {
  return _select;
}


bool peano4::datamanagement::VertexMarker::hasBeenRefined() const {
  return hasBeenRefined(_select);
}


bool peano4::datamanagement::VertexMarker::hasBeenRefined(int i) const {
  return _hasBeenRefined[i];
}


bool peano4::datamanagement::VertexMarker::willBeRefined() const {
  return willBeRefined(_select);
}


bool peano4::datamanagement::VertexMarker::willBeRefined(int i) const {
  return _willBeRefined[i];
}


bool peano4::datamanagement::VertexMarker::isLocal() const {
  return isLocal(_select);
}


bool peano4::datamanagement::VertexMarker::isParentCellLocal() const {
  return _isParentCellLocal;
}


bool peano4::datamanagement::VertexMarker::isLocal(int i) const {
  return _isLocal[i];
}


bool peano4::datamanagement::VertexMarker::isContainedInAdjacentCells( const tarch::la::Vector<Dimensions,double>& x, double scaleAdjacentCells, double tolerance ) const {
  bool  result = true;
  for (int d=0; d<Dimensions; d++) {
    result &= tarch::la::smallerEquals( std::abs(x(d) - (this->x())(d)), scaleAdjacentCells * _h(d), tolerance);
  }
  return result;
}


/*
bool peano4::datamanagement::VertexMarker::isAdjacentToParallelDomainBoundary() const {
  return isAdjacentToParallelDomainBoundary(_select);
}


bool peano4::datamanagement::VertexMarker::isAdjacentToParallelDomainBoundary(int i) const {
  return _isAdjacentToParallelDomainBoundary[i];
}
*/


tarch::la::Vector<Dimensions,int> peano4::datamanagement::VertexMarker::getRelativeAdjacentCell( const tarch::la::Vector<Dimensions,double>& x ) const {
  tarch::la::Vector<Dimensions,int> result;
  for (int d=0; d<Dimensions; d++) {
    result(d) = x(d) <= (this->x())(d) ? 0 : 1;
  }
  return result;
}


bool peano4::datamanagement::VertexMarker::isHanging() const {
  return isHanging( _select );
}


bool peano4::datamanagement::VertexMarker::isHanging(int i) const {
  return _isHanging[i];
}

