// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Phase.h"

namespace runner {
  class GridConstruction;
} // namespace runner

class runner::GridConstruction final : public runner::Phase {
public:
  GridConstruction(runner::PhaseContext& context, int createGridStep);

  void run() override;

private:
  struct ColumnWidths {
    int level{0};
    int leaves{0};
    int efficiency{0};
    int overhead{0};
  };

  runner::PhaseContext& _context;
  int                   _createGridStep{-1};
  ColumnWidths          _widths{};
  bool                  _tableStarted{false};

  std::string row(
    const std::string& level,
    const std::string& leaves,
    const std::string& efficiency,
    const std::string& overhead,
    const std::string& load
  ) const;

  void logRow(
    int           level,
    int           maximumLevel,
    std::uint64_t numberOfLeaves,
    double        replicationOverhead,
    std::uint64_t minimumRankLoad,
    std::uint64_t maximumRankLoad
  );
};
