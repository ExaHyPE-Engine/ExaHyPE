// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridTraversalEvent.h"

static_assert(std::is_standard_layout_v<peano4::grid::GridTraversalEvent>);
static_assert(std::is_trivially_copyable_v<peano4::grid::GridTraversalEvent>);

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridTraversalEvent& event) {
  return (out << event.toString());
}

std::string peano4::grid::GridTraversalEvent::toString() const {
  std::ostringstream out;
  out << "GridTraversalEvent{";

  out << "x=[";
  for (auto i = 0; i < x.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << std::setprecision(6) << x[i];
  }
  out << "]";

  out << ", h=[";
  for (auto i = 0; i < h.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << std::setprecision(6) << h[i];
  }
  out << "]";

  out << "}";

  return out.str();
}

peano4::dataview::CellMarker peano4::grid::GridTraversalEvent::getCellMarker() const {
  peano4::dataview::CellMarker marker;

  marker.level                            = level;
  marker.h                                = h;
  marker.centre                           = x;
  marker.relativePositionWithinFatherCell = relativePositionToFather;

  auto setFlag = [&](const bool condition, const std::uint16_t flag) {
    if (condition) {
      marker.flags |= flag;
    }
  };

  setFlag(hasBeenRefined != 0, peano4::dataview::CellMarker::HasBeenRefined);
  setFlag(willBeRefined != 0, peano4::dataview::CellMarker::WillBeRefined);
  setFlag(isCellLocal != 0, peano4::dataview::CellMarker::IsLocal);
  setFlag(isParentCellLocal != 0, peano4::dataview::CellMarker::IsParentLocal);

  return marker;
}

peano4::dataview::FaceMarker peano4::grid::GridTraversalEvent::getFaceMarker() const {
  peano4::dataview::FaceMarker marker{};

  marker.h                                = h;
  marker.cellCentre                       = x;
  marker.selectedFace                     = 0;
  marker.cellIsLocal                      = (isCellLocal != 0);
  marker.relativePositionWithinFatherCell = relativePositionToFather;

  marker.isLocalMask   = isFaceLocal;
  marker.isHangingMask = isFaceHanging;

  marker.hasBeenRefinedMask = 0;
  marker.willBeRefinedMask  = 0;

  for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {
    bool hasRefined = false;
    bool willRefine = false;

    const int normal = faceNumber % DIMENSIONS;
    for (int i = 0; i < TWO_POWER_D; i++) {
      std::bitset<DIMENSIONS> studiedVertex(static_cast<unsigned long>(i));
      studiedVertex.set(normal, faceNumber >= DIMENSIONS);
      const int vertexNumber = static_cast<int>(studiedVertex.to_ulong());
      hasRefined |= bitTest8(hasBeenRefined, vertexNumber);
      willRefine |= bitTest8(willBeRefined, vertexNumber);
    }

    marker.hasBeenRefinedMask |= static_cast<std::uint8_t>(hasRefined) << faceNumber;
    marker.willBeRefinedMask |= static_cast<std::uint8_t>(willRefine) << faceNumber;
  }

  return marker;
}
