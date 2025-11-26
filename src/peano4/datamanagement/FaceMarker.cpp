// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceMarker.h"

static_assert(std::is_standard_layout_v<peano4::datamanagement::FaceMarker>);
static_assert(std::is_trivially_copyable_v<peano4::datamanagement::FaceMarker>);

std::ostream& operator<<(std::ostream& out, const peano4::datamanagement::FaceMarker& marker) {
  out << marker.toString();
  return out;
}

std::string peano4::datamanagement::FaceMarker::toString() const {
  std::ostringstream msg;
  msg << "FaceMarker{";
  msg << "CellCentre=" << cellCentre;
  msg << ",h=" << h;
  msg << ",SelectedFace=" << selectedFace;
  msg << ",IsCellLocal=" << cellIsLocal;
  msg << ",IsFaceLocal=" << isLocal();
  msg << ",HasBeenRefined=" << hasBeenRefined();
  msg << ",WillBeRefined=" << willBeRefined();
  msg << ",RelPosWithinFather=" << relativePositionWithinFatherCell;
  msg << "}";
  return msg.str();
}
