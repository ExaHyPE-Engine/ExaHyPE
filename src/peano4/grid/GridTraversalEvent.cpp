// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridTraversalEvent.h"

#include "GridTraversalObserver.h"
#include "tarch/services/ServiceRepository.h"

#include <iomanip>
#include <sstream>

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

peano4::datamanagement::CellMarker peano4::grid::GridTraversalEvent::getCellMarker() const {
  datamanagement::CellMarker marker;

  marker.level                            = level;
  marker.h                                = h;
  marker.centre                           = x;
  marker.relativePositionWithinFatherCell = relativePositionToFather;

  auto setFlag = [&](bool cond, std::uint16_t flag) {
    if (cond) {
      marker.flags |= flag;
    }
  };

  setFlag(hasBeenRefined != 0, datamanagement::CellMarker::HasBeenRefined);
  setFlag(willBeRefined != 0, datamanagement::CellMarker::WillBeRefined);
  setFlag(isCellLocal != 0, datamanagement::CellMarker::IsLocal);
  setFlag(isParentCellLocal != 0, datamanagement::CellMarker::IsParentLocal);
  setFlag(isVertexAdjacentToParallelDomainBoundary == 0, datamanagement::CellMarker::AllVerticesInsideDomain);

  const bool oneVertexHanging            = (isVertexHanging != 0);
  bool       oneVertexCreatedOrDestroyed = false;

  for (int i = 0; i < TWO_POWER_D; ++i) {
    const int to   = vertexDataTo[i];
    const int from = vertexDataFrom[i];

    oneVertexCreatedOrDestroyed |= (to == GridTraversalObserver::CreateOrDestroyGridEntity);
    oneVertexCreatedOrDestroyed |= (from == GridTraversalObserver::CreateOrDestroyGridEntity);
  }

  if (oneVertexHanging) {
    marker.flags |= datamanagement::CellMarker::OneVertexHanging;
  }
  if (oneVertexCreatedOrDestroyed) {
    marker.flags |= datamanagement::CellMarker::OneVertexCreatedOrDestroyed;
  }

  const bool couldBeEnclave = (marker.flags & datamanagement::CellMarker::AllVerticesInsideDomain)
                              and not oneVertexHanging;

  if (couldBeEnclave and not(marker.flags & datamanagement::CellMarker::WillBeRefined)
      and not oneVertexCreatedOrDestroyed) {
    marker.flags |= datamanagement::CellMarker::WillBeEnclave;
  }
  if (couldBeEnclave and not(marker.flags & datamanagement::CellMarker::HasBeenRefined)
      and not oneVertexCreatedOrDestroyed) {
    marker.flags |= datamanagement::CellMarker::HasBeenEnclave;
  }

  return marker;
}

peano4::datamanagement::FaceMarker peano4::grid::GridTraversalEvent::getFaceMarker() const {
  peano4::datamanagement::FaceMarker marker{};

  marker.h                                = h;
  marker.cellCentre                       = x;
  marker.selectedFace                     = 0;
  marker.cellIsLocal                      = (isCellLocal != 0);
  marker.relativePositionWithinFatherCell = relativePositionToFather;

  marker.isLocalMask   = isFaceLocal;
  marker.isHangingMask = isFaceHanging;

  marker.hasBeenRefinedMask = 0;
  marker.willBeRefinedMask  = 0;

  for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; ++faceNumber) {
    bool hasRefined = false;
    bool willRefine = false;

    const int normal = faceNumber % DIMENSIONS;
    for (int i = 0; i < TWO_POWER_D; ++i) {
      std::bitset<DIMENSIONS> studiedVertex(static_cast<unsigned long>(i));
      studiedVertex.set(normal, faceNumber >= DIMENSIONS);
      const int v = static_cast<int>(studiedVertex.to_ulong());
      hasRefined |= bitTest8(hasBeenRefined, v);
      willRefine |= bitTest8(willBeRefined, v);
    }

    marker.hasBeenRefinedMask |= static_cast<std::uint8_t>(hasRefined) << faceNumber;
    marker.willBeRefinedMask |= static_cast<std::uint8_t>(willRefine) << faceNumber;
  }

  return marker;
}
