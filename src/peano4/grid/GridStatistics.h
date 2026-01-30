// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace peano4::grid {
  struct GridStatistics;
} // namespace peano4::grid

struct peano4::grid::GridStatistics {
  int maxDepth = -1;
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

  tarch::la::Vector<DIMENSIONS, double> minH = tarch::la::Vector<DIMENSIONS, double>(std::numeric_limits<double>::max());

  GridStatistics() noexcept = default;
  enum class ObjectConstruction { NoData };
  GridStatistics(ObjectConstruction) noexcept:
    GridStatistics() {}

  inline bool getCoarseningHasBeenVetoed() const noexcept { return (flags & CoarseningHasBeenVetoed) != 0; }
  inline void setCoarseningHasBeenVetoed(bool v) noexcept {
    v ? (flags |= CoarseningHasBeenVetoed) : (flags &= ~CoarseningHasBeenVetoed);
  }

  inline bool getRemovedEmptySubtree() const noexcept { return (flags & RemovedEmptySubtree) != 0; }
  inline void setRemovedEmptySubtree(bool v) noexcept {
    v ? (flags |= RemovedEmptySubtree) : (flags &= ~RemovedEmptySubtree);
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
