// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "peano4/RefinementRequest.h"
#include "tarch/Span.h"

namespace peano4 {
  class Partition;
} // namespace peano4

namespace exahype2 {
  class RefinementRequests;
} // namespace exahype2

/**
 * Merges the refinement votes of every solver of a run, one vote per cell slot, and turns them
 * into the refinement requests of the spacetree: A refine vote names its cell, and the erase
 * votes of all children of one parent name the parent.
 */
class exahype2::RefinementRequests final {
public:
  void clear(int numberOfSlots);

  void merge(const std::vector<exahype2::RefinementCommand>& commands, tarch::Span<const int> cells);
  void merge(exahype2::RefinementCommand command, tarch::Span<const int> cells);

  [[nodiscard]] std::vector<peano4::RefinementRequest> getRefinementRequests(const peano4::Partition& partition) const;

private:
  static constexpr std::uint8_t NoVote = 255;

  std::vector<std::uint8_t> _votes{};
};
