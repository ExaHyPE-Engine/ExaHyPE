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
 * Realises the committed refine and erase requests between time steps. The phase sweeps until the
 * grid reports itself stationary, so a caller resumes on a settled topology. It collects what it
 * did across its invocations and writes the collection when a caller asks for it.
 */
class runner::DynamicAMR final : public runner::Phase {
public:
  DynamicAMR(runner::PhaseContext& context, const int dynamicAMRStep);

  void run() override;

  bool isEnabled() const;
  bool hasPendingMeshUpdates() const;
  bool hasUndecidedRefinementCriterion() const;

  static bool resolvePendingMeshUpdates(const runner::RepositoryCallbacks& repositories);

  void reportPendingSweeps();

private:
  friend class runner::tests::DynamicAMRTest;

  /** Wall-clock seconds between the progress reports one drain writes. */
  static constexpr double SecondsBetweenReports = 5.0;

  /** Bounds the sweeps of one drain; a drain that reaches this cap stops the run with an error. */
  static constexpr int MaximumSweeps = 1024;

  /**
   * Records the drains a report line covers with their sweeps, the grid they
   * left behind, and both load balances before and after.
   */
  struct Activity {
    int           regrids{0};
    int           sweeps{0};
    double        wallClock{0.0};
    unsigned long leavesBefore{0};
    unsigned long leavesAfter{0};
    int           minimumLevel{0};
    int           maximumLevel{0};

    int           numberOfRanks{1};
    double        interRankBalanceBefore{0.0};
    double        interRankBalanceAfter{0.0};
    unsigned long minimumRankLeaves{0};
    unsigned long maximumRankLeaves{0};
    double        replicationOverhead{0.0};

    unsigned long subtreeTasksBefore{0};
    unsigned long subtreeTasksAfter{0};
    unsigned long minimumTaskCells{0};
    unsigned long maximumTaskCells{0};
    double        threadUtilisation{0.0};
  };

  runner::PhaseContext& _context;
  int                   _dynamicAMRStep{-1};
  Activity              _activity{};
  /** Stores the task time accumulated when the current drain begins. */
  double _taskSecondsAtStart{0.0};

  double                          accumulatedTaskSeconds() const;
  void                            startActivity();
  void                            reportActivity();
  static std::vector<std::string> describe(const runner::DynamicAMR::Activity& activity);
};
