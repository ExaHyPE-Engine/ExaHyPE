// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Phase.h"

#include <string>
#include <vector>

namespace runner {
  class GridInitialisation;
} // namespace runner

class runner::GridInitialisation final : public runner::Phase {
public:
  GridInitialisation(PhaseContext& context, int initGridStep);

  std::vector<std::string> widthSamples() const override;
  void                     run() override;

private:
  PhaseContext& _context;
  int           _initGridStep;
};
