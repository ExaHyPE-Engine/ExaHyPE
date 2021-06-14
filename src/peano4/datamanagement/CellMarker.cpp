#include "CellMarker.h"

#include "peano4/grid/TraversalObserver.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/utils/Globals.h"


std::ostream& operator<<( std::ostream& out, const peano4::datamanagement::CellMarker& marker ) {
  out << marker.toString();
  return out;
}


peano4::datamanagement::CellMarker::CellMarker(
  const peano4::grid::GridTraversalEvent& event
):
  _centre(event.getX()),
  _h(event.getH()),
  _isRefined(event.getIsRefined()!=0),
  _isLocal(event.getIsCellLocal()),
  _areAllVerticesRefined( event.getIsRefined().all() ),
  _isOneVertexHanging( false ),
  _areAllVerticesInsideDomain( event.getIsVertexInsideDomain().all() ),
  _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing( event.getInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing() )
{
  for (int i=0; i<TwoPowerD; i++) {
    _isOneVertexHanging |= event.getVertexDataTo(i)  ==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity;
    _isOneVertexHanging |= event.getVertexDataFrom(i)==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity;
  }
  _relativePositionOfCellWithinFatherCell = event.getRelativePositionToFather();
}


tarch::la::Vector<Dimensions,int>  peano4::datamanagement::CellMarker::getRelativePositionWithinFatherCell() const {
  return _relativePositionOfCellWithinFatherCell;
}


bool peano4::datamanagement::CellMarker::isContained( const tarch::la::Vector<Dimensions,double>& x ) const {
  bool result = true;
  for (int d=0; d<Dimensions; d++) {
    result &= x(d)>=_centre(d)-_h(d)/2.0;
    result &= x(d)<=_centre(d)+_h(d)/2.0;
  }
  return result;
}


bool peano4::datamanagement::CellMarker::overlaps( const tarch::la::Vector<Dimensions,double>& offset, const tarch::la::Vector<Dimensions,double>& size ) const {
  bool overlaps = true;

  for (int d=0; d<Dimensions; d++) {
    overlaps &= tarch::la::smallerEquals( _centre(d)-_h(d)/2.0, offset(d)+size(d) );
    overlaps &= tarch::la::greaterEquals( _centre(d)+_h(d)/2.0, offset(d) );
  }

  return overlaps;
}


bool peano4::datamanagement::CellMarker::isRefined() const {
  return _isRefined;
}


bool peano4::datamanagement::CellMarker::isLocal() const {
  return _isLocal;
}


tarch::la::Vector<Dimensions,double>  peano4::datamanagement::CellMarker::x() const {
  return _centre;
}


tarch::la::Vector<Dimensions,double>  peano4::datamanagement::CellMarker::h() const {
  return _h;
}


tarch::la::Vector<Dimensions,double>  peano4::datamanagement::CellMarker::getOffset() const {
  return _centre-0.5*_h;
}


std::string peano4::datamanagement::CellMarker::toString() const {
  std::ostringstream msg;
  msg << "(x=" << _centre << ",h=" << _h << ",refined=" << _isRefined << ")";
  return msg.str();
}


/*
bool peano4::datamanagement::CellMarker::areAllVerticesRefined() const {
  return _areAllVerticesRefined;
}


bool peano4::datamanagement::CellMarker::isOneVertexHanging() const {
  return _isOneVertexHanging;
}


bool peano4::datamanagement::CellMarker::isAdjacentToDomainBoundary() const {
  return _isAdjacentToDomainBoundary;
}
*/



bool peano4::datamanagement::CellMarker::isEnclaveCell() const {
  return not isSkeletonCell();
}


bool peano4::datamanagement::CellMarker::isSkeletonCell() const {
  return not _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing or not _areAllVerticesInsideDomain or _isOneVertexHanging or not (_areAllVerticesRefined or not _isRefined);
}
