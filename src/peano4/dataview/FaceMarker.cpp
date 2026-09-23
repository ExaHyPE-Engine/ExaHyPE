// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceMarker.h"

static_assert(std::is_standard_layout_v<peano4::dataview::FaceMarker>);
static_assert(std::is_trivially_copyable_v<peano4::dataview::FaceMarker>);

std::ostream& peano4::dataview::operator<<(std::ostream& out, const peano4::dataview::FaceMarker& marker) {
  out << marker.toString();
  return out;
}

std::string peano4::dataview::FaceMarker::toString() const {
  std::ostringstream out;
  out << std::boolalpha;
  out << "FaceMarker{";
  out << "cellCentre=" << ::toString(cellCentre);
  out << ",h=" << ::toString(h);
  out << ",selectedFace=" << selectedFace;
  out << ",cellIsLocal=" << cellIsLocal;
  out << ",cellHasBeenRefined=" << cellHasBeenRefined;
  out << ",cellWillBeRefined=" << cellWillBeRefined;
  out << ",isLocal=" << isLocal();
  out << ",hasBeenRefined=" << hasBeenRefined();
  out << ",willBeRefined=" << willBeRefined();
  out << ",relativePositionWithinFatherCell=" << ::toString(relativePositionWithinFatherCell);
  out << "}";
  return out.str();
}
