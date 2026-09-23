// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellMarker.h"

static_assert(std::is_standard_layout_v<peano4::dataview::CellMarker>);
static_assert(std::is_trivially_copyable_v<peano4::dataview::CellMarker>);

std::ostream& peano4::dataview::operator<<(std::ostream& out, const peano4::dataview::CellMarker& marker) {
  out << marker.toString();
  return out;
}

std::string peano4::dataview::CellMarker::toString() const {
  std::ostringstream out;
  out << std::boolalpha;
  out << "CellMarker{";
  out << "centre=" << ::toString(centre);
  out << ",h=" << ::toString(h);
  out << ",hasBeenRefined=" << hasBeenRefined();
  out << ",willBeRefined=" << willBeRefined();
  out << ",isLocal=" << isLocal();
  out << ",isParentLocal=" << isParentLocal();
  out << "}";
  return out.str();
}
