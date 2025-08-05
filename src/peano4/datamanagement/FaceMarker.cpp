// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceMarker.h"

std::ostream& operator<<(std::ostream& out, const peano4::datamanagement::FaceMarker& marker) {
  out << marker.toString();
  return out;
}

peano4::datamanagement::FaceMarker::FaceMarker(const peano4::grid::GridTraversalEvent& event, int select):
  _cellCentre(event.getX()),
  _h(event.getH()),
  _cellIsLocal(event.getIsCellLocal()),
  _select(select),
  _isLocal(event.getIsFaceLocal()),
  _relativePositionOfCellWithinFatherCell(event.getRelativePositionToFather()) {
  for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {
    _hasBeenRefined[faceNumber] = false;
    _willBeRefined[faceNumber]  = false;

    const int normal = faceNumber % DIMENSIONS;
    for (int i = 0; i < TWO_POWER_D; i++) {
      std::bitset<DIMENSIONS> studiedVertex = i;
      studiedVertex.set(normal, faceNumber >= DIMENSIONS);
      _hasBeenRefined.set(faceNumber, _hasBeenRefined[faceNumber] or event.getHasBeenRefined(studiedVertex.to_ulong()));
      _willBeRefined.set(faceNumber, _willBeRefined[faceNumber] or event.getWillBeRefined(studiedVertex.to_ulong()));
    }
  }
}

std::string peano4::datamanagement::FaceMarker::toString() const {
  std::ostringstream msg;
  msg
    << "(cell-centre=" << _cellCentre << ",cell-h=" << _h << ",select=" << _select << ",is-cell-local=" << _cellIsLocal
    << ",is-face-local=" << isLocal() << ",has-face-been-refined=" << hasBeenRefined()
    << ",will-face-be-refined=" << willBeRefined() << ",rel-pos=" << _relativePositionOfCellWithinFatherCell << ")";
  return msg.str();
}

tarch::la::Vector<DIMENSIONS, int> peano4::datamanagement::FaceMarker::getRelativePositionWithinFatherCell() const {
  assertion1(_select >= 0, toString());
  return getRelativePositionWithinFatherCell(_select);
}

tarch::la::Vector<DIMENSIONS, int> peano4::datamanagement::FaceMarker::getRelativePositionWithinFatherCell(
  int i
) const {
  assertion2(i >= 0, i, toString());
  tarch::la::Vector<DIMENSIONS, int> result = _relativePositionOfCellWithinFatherCell;
  const int                          normal = i % DIMENSIONS;
  result(normal) += i >= DIMENSIONS ? 1 : 0;
  return result;
}

tarch::la::Vector<DIMENSIONS - 1, int> peano4::datamanagement::FaceMarker::getRelativePositionWithinFatherFace() const {
  tarch::la::Vector<DIMENSIONS - 1, int> result;
  const int                              normal = getSelectedFaceNumber() % DIMENSIONS;
  int                                    pos    = 0;
  for (int d = 0; d < DIMENSIONS; d++) {
    if (d != normal) {
      result[pos] = _relativePositionOfCellWithinFatherCell[d];
      pos++;
    }
  }
  return result;
}

bool peano4::datamanagement::FaceMarker::isInteriorFaceWithinPatch() const {
  int normal = getSelectedFaceNumber() % DIMENSIONS;
  return getRelativePositionWithinFatherCell()(normal) == 1 or getRelativePositionWithinFatherCell()(normal) == 2;
}
