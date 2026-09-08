// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace peano4::grid {
  struct GridStatistics;
} // namespace peano4::grid

struct peano4::grid::GridStatistics {
  int maxDepth = 0;
  int minDepth = std::numeric_limits<int>::max();

  int numberOfLocalUnrefinedCells  = 0;
  int numberOfRemoteUnrefinedCells = 0;

  int numberOfLocalRefinedCells  = 0;
  int numberOfRemoteRefinedCells = 0;

  int numberOfVertices        = 0;
  int numberOfHangingVertices = 0;

  int numberOfStationarySweeps = 0;

  std::uint8_t flags = 0;

  static constexpr std::uint8_t CoarseningHasBeenVetoed = 1 << 0;
  static constexpr std::uint8_t RemovedEmptySubtree     = 1 << 1;

  tarch::la::Vector<DIMENSIONS, double> minH =
    tarch::la::Vector<DIMENSIONS, double>(std::numeric_limits<double>::max());

  GridStatistics() noexcept = default;

  INLINE_METHOD bool getCoarseningHasBeenVetoed() const noexcept { return (flags & CoarseningHasBeenVetoed) != 0; }
  INLINE_METHOD void setCoarseningHasBeenVetoed(bool value) noexcept {
    value ? (flags |= CoarseningHasBeenVetoed) : (flags &= ~CoarseningHasBeenVetoed);
  }

  INLINE_METHOD bool getRemovedEmptySubtree() const noexcept { return (flags & RemovedEmptySubtree) != 0; }
  INLINE_METHOD void setRemovedEmptySubtree(bool value) noexcept {
    value ? (flags |= RemovedEmptySubtree) : (flags &= ~RemovedEmptySubtree);
  }

  void        clear(bool isGlobalMasterTree);
  std::size_t getMemoryUsageInBytes() const;

  std::string toString() const;
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridStatistics& statistics);

peano4::grid::GridStatistics operator+(
  const peano4::grid::GridStatistics& lhs,
  const peano4::grid::GridStatistics& rhs
);
peano4::grid::GridStatistics operator+=(peano4::grid::GridStatistics& lhs, const peano4::grid::GridStatistics& rhs);
