#include "FaceMarker.h"


std::ostream& operator<<( std::ostream& out, const peano4::datamanagement::FaceMarker& marker ) {
  out << marker.toString();
  return out;
}


peano4::datamanagement::FaceMarker::FaceMarker(
  const peano4::grid::GridTraversalEvent&   event,
  int                                       select
):
  _cellCentre(event.getX()),
  _h(event.getH()),
  _cellIsLocal(event.getIsCellLocal()),
  _select(select) {
  for (int faceNumber=0; faceNumber<2*Dimensions; faceNumber++) {
    _hasBeenRefined[faceNumber] = false;
    _willBeRefined[faceNumber]  = false;
    _isLocal[faceNumber]        = event.getIsFaceLocal(faceNumber);

    const int normal = faceNumber % Dimensions;
    for (int i=0; i<TwoPowerD; i++) {
      std::bitset<Dimensions> studiedVertex = i;
      studiedVertex.set(normal,faceNumber>=Dimensions);
      _hasBeenRefined.set( faceNumber, _hasBeenRefined[faceNumber] or event.getHasBeenRefined(studiedVertex.to_ulong()) );
      _willBeRefined.set(  faceNumber, _willBeRefined[faceNumber]  or event.getWillBeRefined(studiedVertex.to_ulong()) );
    }
  }
  _relativePositionOfCellWithinFatherCell = event.getRelativePositionToFather();
}


tarch::la::Vector<Dimensions,double> peano4::datamanagement::FaceMarker::x(int i) const {
  tarch::la::Vector<Dimensions,double> result( _cellCentre );
  int normal = i % Dimensions;
  result(normal) += i >= Dimensions ? _h(normal)/2.0 : -_h(normal)/2.0;
  return result;
}


tarch::la::Vector<Dimensions,double> peano4::datamanagement::FaceMarker::x() const {
  return x(_select);
}


tarch::la::Vector<Dimensions,double> peano4::datamanagement::FaceMarker::normal(int i) const {
  tarch::la::Vector<Dimensions,double> result( 0.0 );
  int index = i % Dimensions;
  result(index) += i<Dimensions ? -_h(index) : _h(index);
  return result;
}


tarch::la::Vector<Dimensions,double> peano4::datamanagement::FaceMarker::normal() const {
  return normal(_select);
}


tarch::la::Vector<Dimensions,double> peano4::datamanagement::FaceMarker::outerNormal(int i) const {
  tarch::la::Vector<Dimensions,double> result( 0.0 );
  int  index        = i % Dimensions;
  bool negativeSign = i<Dimensions;
  if ( not _cellIsLocal ) negativeSign = not negativeSign;
  result(index) += negativeSign ? - _h(index) : _h(index);
  return result;
}


tarch::la::Vector<Dimensions,double> peano4::datamanagement::FaceMarker::outerNormal() const {
  return outerNormal(_select);
}


std::string peano4::datamanagement::FaceMarker::toString() const {
  std::ostringstream msg;
  msg << "(x=" << _cellCentre
      << ",h=" << _h
      << ",select=" << _select
      << ",is-cell-local=" << _cellIsLocal
      << ",is-face-local=" << isLocal()
      << ",has-face-been-refined=" << hasBeenRefined()
      << ",will-face-be-refined=" << willBeRefined()
      << ")";
  return msg.str();
}


tarch::la::Vector<Dimensions,double>  peano4::datamanagement::FaceMarker::h() const {
  return _h;
}


peano4::datamanagement::FaceMarker& peano4::datamanagement::FaceMarker::select(int face) {
  _select = face;
  return *this;
}


int peano4::datamanagement::FaceMarker::getSelectedFaceNumber() const {
  return _select;
}


bool peano4::datamanagement::FaceMarker::hasBeenRefined(int i) const {
  return _hasBeenRefined[i];
}


bool peano4::datamanagement::FaceMarker::hasBeenRefined() const {
  return hasBeenRefined(_select);
}


bool peano4::datamanagement::FaceMarker::willBeRefined(int i) const {
  return _willBeRefined[i];
}


bool peano4::datamanagement::FaceMarker::willBeRefined() const {
  return willBeRefined(_select);
}


bool peano4::datamanagement::FaceMarker::isLocal(int i) const {
  return _isLocal[i];
}


bool peano4::datamanagement::FaceMarker::isLocal() const {
  return isLocal(_select);
}


tarch::la::Vector<Dimensions,int>  peano4::datamanagement::FaceMarker::getRelativePositionWithinFatherCell() const {
  return getRelativePositionWithinFatherCell(_select);
}


tarch::la::Vector<Dimensions,int>  peano4::datamanagement::FaceMarker::getRelativePositionWithinFatherCell(int i) const {
  tarch::la::Vector<Dimensions,int> result = _relativePositionOfCellWithinFatherCell;
  const int normal = i % Dimensions;
  result(normal) += i>=Dimensions ? 1 : 0;
  return result;
}

