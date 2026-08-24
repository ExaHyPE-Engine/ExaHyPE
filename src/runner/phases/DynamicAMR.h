// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Phase.h"

namespace runner {
  class DynamicAMR;
} // namespace runner

namespace runner::tests {
  class DynamicAMRTest;
} // namespace runner::tests

/** Realises committed refinement and coarsening requests between time steps. */
class runner::DynamicAMR final : public runner::Phase {
public:
  DynamicAMR(runner::PhaseContext& context, const int dynamicAMRStep);

  void run() override;

  bool isEnabled() const;
  bool hasPendingMeshUpdates() const;
  bool hasUndecidedRefinementCriterion() const;

  void reportPendingSweeps();

private:
  friend class runner::tests::DynamicAMRTest;

  /**
   * Sweeps a single drain may run before it reports its progress. Ordinary
   * drains stay below this, so their collected activity only surfaces with the
   * next time-step block; a drain beyond it is in trouble and writes a line
   * before the MaxSweeps abort, which tells a slow drain from a hang.
   */
  static constexpr int SweepsPerReport = 32;

  /**
   * Bound on one drain. One committed event set realises within a handful of sweeps, so a
   * drain that reaches this cap means the grid cannot settle and the run stops with an error.
   */
  static constexpr int MaxSweeps = 64;

  /** The sweeps a report line covers, and the grid they left behind. */
  struct Activity {
    int           sweeps       = 0;
    double        wallClock    = 0.0;
    unsigned long leavesBefore = 0;
    unsigned long leavesAfter  = 0;
    int           minLevel     = 0;
    int           maxLevel     = 0;
  };

  runner::PhaseContext& _context;
  int                   _dynamicAMRStep;
  Activity              _activity;

  void               startActivity();
  void               reportActivity();
  static std::string describe(const Activity& activity);
};
