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
  GridInitialisation(runner::PhaseContext& context, const int initGridStep, runner::DynamicAMR& dynamicAMR);

  void run() override;

private:
  /**
   * Bound on the initialisation rounds. Every round realises at least one further
   * refinement level, so more rounds than any admissible level span means the
   * criterion keeps changing its mind; the run then stops, so the time loop never
   * starts with mesh changes still outstanding.
   */
  static constexpr int MaxRounds = 16;

  runner::PhaseContext& _context;
  int                   _initGridStep;
  runner::DynamicAMR&   _dynamicAMR;
};
