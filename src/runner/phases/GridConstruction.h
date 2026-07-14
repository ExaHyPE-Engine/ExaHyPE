// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Phase.h"

namespace runner {
  class GridConstruction;
} // namespace runner

class runner::GridConstruction final : public runner::Phase {
public:
  GridConstruction(PhaseContext& context, int createGridStep, int loadBalancingStep);

  std::vector<std::string> widthSamples() const override;
  void                     run() override;

private:
  struct ColumnWidths {
    int level;
    int leaves;
    int trees;
    int efficiency;
    int overhead;
  };

  PhaseContext& _context;
  int           _createGridStep;
  int           _loadBalancingStep;
  ColumnWidths  _widths;
  bool          _tableStarted = false;

  std::string row(
    const std::string& level,
    const std::string& leaves,
    const std::string& trees,
    const std::string& efficiency,
    const std::string& overhead,
    const std::string& load
  ) const;

  void logRow(
    int           level,
    int           maxLevel,
    unsigned long leaves,
    unsigned long outer,
    unsigned long trees,
    unsigned long minLoad,
    unsigned long maxLoad
  );
};
