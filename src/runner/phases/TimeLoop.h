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
  TimeLoop(runner::PhaseContext& context, const int timeStepStep, const int plotStep, runner::DynamicAMR& dynamicAMR);

  void run() override;

  /** True if the run ended early on a non-critical assertion (one final plot was written). */
  bool stoppedAfterNonCriticalAssertion() const { return _stoppedAfterNonCriticalAssertion; }

private:
  runner::PhaseContext& _context;
  int                   _timeStepStep;
  int                   _plotStep;
  runner::DynamicAMR&   _dynamicAMR;
  bool                  _stoppedAfterNonCriticalAssertion = false;

  /**
   * Wall clock in seconds of every completed time step, the first one included.
   */
  double _timeSteppingSeconds = 0.0;

  /**
   * Wall clock in seconds that the DynamicAMR measurement holds when the loop starts. Grid
   * initialisation runs through the same measurement, and the estimate subtracts this offset.
   */
  double _dynamicAMRSecondsBeforeLoop = 0.0;

  void logTimeStep(
    const double        minimumTimeStamp,
    const double        maximumTimeStamp,
    const double        terminalTime,
    const double        wallClockSeconds,
    const unsigned long completedSteps,
    const int           maximumSteps
  );
  void logPlot(const int snapshot, const int numberOfSnapshots, const double timeStamp, const double wallClockSeconds);
  std::string formatETA(const double fraction) const;
};
