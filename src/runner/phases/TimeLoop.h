// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Phase.h"

namespace runner {
  class DynamicAMR;
  class TimeLoop;
} // namespace runner

class runner::TimeLoop final : public runner::Phase {
public:
  TimeLoop(
    runner::PhaseContext& context,
    int                   timeStepStep,
    int                   plotStep,
    runner::DynamicAMR&   dynamicAMR,
    int                   checkpointStep
  );

  void run() override;

  /** True if the run ended early on a non-critical assertion (one final plot was written). */
  bool stoppedAfterNonCriticalAssertion() const { return _stoppedAfterNonCriticalAssertion; }

private:
  runner::PhaseContext& _context;
  int                   _timeStepStep;
  int                   _plotStep;
  runner::DynamicAMR&   _dynamicAMR;
  int                   _checkpointStep;
  bool                  _stoppedAfterNonCriticalAssertion{false};

  /**
   * Wall clock in seconds of every completed time step, the first one included.
   */
  double _timeSteppingSeconds{0.0};

  /**
   * Wall clock in seconds that the DynamicAMR measurement holds when the loop starts. Grid
   * initialisation runs through the same measurement, and the estimate subtracts this offset.
   */
  double _dynamicAMRSecondsBeforeLoop{0.0};

  void logTimeStep(
    double        minimumTimeStamp,
    double        maximumTimeStamp,
    double        terminalTime,
    double        wallClockSeconds,
    unsigned long completedSteps,
    int           maximumSteps
  );
  void        logPlot(int snapshot, int numberOfSnapshots, double timeStamp, double wallClockSeconds);
  void        logCheckpoint(int checkpoint, int numberOfCheckpoints, double timeStamp, double wallClockSeconds);
  std::string formatETA(double fraction) const;
};
