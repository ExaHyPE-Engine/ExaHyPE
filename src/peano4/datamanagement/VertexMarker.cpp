#include "VertexMarker.h"


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
  for (int i=0; i<TwoPowerD; i++) {
    _isRefined[i]  = event.getIsRefined(i);
    _isLocal[i]    = event.getIsVertexLocal(i);
  }
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
  msg << "(" << _cellCentre << "," << _h << ")";
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


bool peano4::datamanagement::VertexMarker::isRefined() const {
  return isRefined(_select);
}


bool peano4::datamanagement::VertexMarker::isRefined(int i) const {
  return _isRefined[i];
}


bool peano4::datamanagement::VertexMarker::isLocal() const {
  return isLocal(_select);
}


bool peano4::datamanagement::VertexMarker::isLocal(int i) const {
  return _isLocal[i];
}


