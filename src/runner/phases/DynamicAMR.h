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

/**
 * Realise committed refine/coarsen requests between time steps.
 *
 * The time loop invokes this phase whenever the completed time step committed
 * grid-control events. The phase drains them on the dedicated DynamicAMR
 * observer - one traversal triggers the topology transition, the next realises
 * the created/destroyed cells and faces - and keeps sweeping until the grid
 * reports itself stationary again, so the time loop always resumes on a
 * settled topology.
 *
 * A regridding scenario invokes the phase after almost every time step, so a
 * report per invocation would bury the time steps. The phase therefore collects
 * what it did, and the time loop asks for the collection before it prints a time
 * step. A single drain that runs past SweepsPerReport sweeps reports
 * every so many sweeps on its own, so a grid that cannot settle keeps writing
 * lines and cannot be mistaken for a hang.
 */
class runner::DynamicAMR final : public runner::Phase {
public:
  DynamicAMR(PhaseContext& context, int dynamicAMRStep);

  std::vector<std::string> widthSamples() const override;
  void                     run() override;

  bool isEnabled() const;
  bool hasPendingMeshUpdates() const;

  /** Report the sweeps collected so far, if any. */
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

  PhaseContext& _context;
  int           _dynamicAMRStep;
  Activity      _activity;

  void               startActivity();
  void               reportActivity();
  static std::string describe(const Activity& activity);
};
