// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Phase.h"

namespace runner {
  class DynamicAMR;
  class GridInitialisation;
} // namespace runner

class runner::GridInitialisation final : public runner::Phase {
public:
  GridInitialisation(PhaseContext& context, int initGridStep, DynamicAMR& dynamicAMR);

  std::vector<std::string> widthSamples() const override;
  void                     run() override;

private:
  /**
   * Bound on the initialisation rounds. Every round realises at least one further
   * refinement level, so more rounds than any admissible level span means the
   * criterion keeps changing its mind; the run then stops, so the time loop never
   * starts with mesh changes still outstanding.
   */
  static constexpr int MaxRounds = 16;

  PhaseContext& _context;
  int           _initGridStep;
  DynamicAMR&   _dynamicAMR;
};
