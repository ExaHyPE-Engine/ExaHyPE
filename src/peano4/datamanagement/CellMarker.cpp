// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellMarker.h"

#include "tarch/services/ServiceRepository.h"

static_assert(std::is_standard_layout_v<peano4::datamanagement::CellMarker>);
static_assert(std::is_trivially_copyable_v<peano4::datamanagement::CellMarker>);

std::ostream& operator<<(std::ostream& out, const peano4::datamanagement::CellMarker& marker) {
  out << marker.toString();
  return out;
}

std::string peano4::datamanagement::CellMarker::toString() const {
  std::ostringstream out;
  out << "CellMarker{";
  out << "Centre=" << centre;
  out << ",h=" << h;
  out << ",HasBeenRefined=" << hasBeenRefined();
  out << ",WillBeRefined=" << willBeRefined();
  out << ",IsLocal=" << isLocal();
  out << ",IsParentLocal=" << isParentLocal();
  out << ",HasBeenEnclave=" << hasBeenEnclaveCell();
  out << ",WillBeEnclave=" << willBeEnclaveCell();
  out << "}";
  return out.str();
}
