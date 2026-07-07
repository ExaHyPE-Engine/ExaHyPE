// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include <vector>

namespace runner {
  struct RepositoryCallbacks;
  class ReportBox;

  void writeGridSnapshot(ReportBox& box, const RepositoryCallbacks& repositories);

  /** Worst-case snapshot lines, so the box is sized to fit them (their values are only known later). */
  std::vector<std::string> gridSnapshotWidthSamples();
} // namespace runner
