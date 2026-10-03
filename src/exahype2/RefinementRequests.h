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
 * Merged refinement votes of all solvers, one per cell slot. A refine vote refines its cell, and
 * the erase votes of all children of one parent erase the parent.
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
