// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace runner {
  struct RepositoryCallbacks;
  class ReportBox;

  std::uint64_t boundedFullyRefinedLeaves(const int maximumLevel);
  std::string   gridCellSummary(const std::uint64_t numberOfLeaves, const std::uint64_t numberOfRefinedCells);

  void writeGridSnapshot(ReportBox& box, const RepositoryCallbacks& callbacks);
} // namespace runner
