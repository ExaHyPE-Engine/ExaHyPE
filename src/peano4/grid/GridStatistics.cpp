// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridStatistics.h"

#include "GridVertex.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

static_assert(std::is_standard_layout_v<peano4::grid::GridStatistics>);
static_assert(std::is_trivially_copyable_v<peano4::grid::GridStatistics>);

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridStatistics& statistics) {
  return (out << statistics.toString());
}

peano4::grid::GridStatistics operator+(
  const peano4::grid::GridStatistics& lhs,
  const peano4::grid::GridStatistics& rhs
) {
  peano4::grid::GridStatistics result;

  result.minDepth                     = std::min(lhs.minDepth, rhs.minDepth);
  result.maxDepth                     = std::max(lhs.maxDepth, rhs.maxDepth);
  result.numberOfLocalUnrefinedCells  = lhs.numberOfLocalUnrefinedCells + rhs.numberOfLocalUnrefinedCells;
  result.numberOfRemoteUnrefinedCells = lhs.numberOfRemoteUnrefinedCells + rhs.numberOfRemoteUnrefinedCells;
  result.numberOfLocalRefinedCells    = lhs.numberOfLocalRefinedCells + rhs.numberOfLocalRefinedCells;
  result.numberOfRemoteRefinedCells   = lhs.numberOfRemoteRefinedCells + rhs.numberOfRemoteRefinedCells;
  result.numberOfVertices             = lhs.numberOfVertices + rhs.numberOfVertices;
  result.numberOfHangingVertices      = lhs.numberOfHangingVertices + rhs.numberOfHangingVertices;
  result.numberOfStationarySweeps     = std::min(lhs.numberOfStationarySweeps, rhs.numberOfStationarySweeps);
  result.minH                         = tarch::la::min(lhs.minH, rhs.minH);
  result.flags                        = static_cast<std::uint8_t>(lhs.flags | rhs.flags);

  return result;
}

peano4::grid::GridStatistics operator+=(peano4::grid::GridStatistics& lhs, const peano4::grid::GridStatistics& rhs) {
  lhs = lhs + rhs;
  return lhs;
}

void peano4::grid::GridStatistics::clear(bool isGlobalMasterTree) {
  minDepth                     = std::numeric_limits<int>::max();
  maxDepth                     = -1;
  numberOfLocalUnrefinedCells  = 0;
  numberOfRemoteUnrefinedCells = 0;
  numberOfLocalRefinedCells    = isGlobalMasterTree ? 1 : 0;
  numberOfRemoteRefinedCells   = isGlobalMasterTree ? 0 : 1;
  numberOfVertices             = 0;
  numberOfHangingVertices      = 0;
  numberOfStationarySweeps     = numberOfStationarySweeps + 1;
  minH                         = tarch::la::Vector<DIMENSIONS, double>(std::numeric_limits<double>::max());
  setRemovedEmptySubtree(false);
  setCoarseningHasBeenVetoed(false);
}

std::size_t peano4::grid::GridStatistics::getMemoryUsageInBytes() const {
  return numberOfVertices * sizeof(GridVertex);
}

std::string peano4::grid::GridStatistics::toString() const {
  std::ostringstream out;
  out << "GridStatistics{";
  out << "Max Depth=" << maxDepth;
  out << ", Min Depth=" << minDepth;
  out << ", Leaves=" << numberOfLocalUnrefinedCells;
  out << ", Remote Leaves=" << numberOfRemoteUnrefinedCells;
  out << ", Vertices=" << numberOfVertices;
  out << ", Hanging Vertices=" << numberOfHangingVertices;
  out << ", MinH=[";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d) {
      out << ",";
    }
    out << std::setprecision(6) << minH[d];
  }
  out << "]";
  const auto memoryUsageInBytes = getMemoryUsageInBytes();
  const auto memoryUsageFormat  = tarch::chooseMemoryUsageFormat(memoryUsageInBytes);
  out << ", Memory=" << std::fixed << std::setprecision(2)
      << tarch::convertMemorySize(memoryUsageInBytes, memoryUsageFormat) << " " << tarch::toString(memoryUsageFormat);
  out << "}";
  return out.str();
}
