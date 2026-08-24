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

  bool terminatedGracefully() const;

private:
  runner::PhaseContext& _context;
  int                   _timeStepStep;
  int                   _plotStep;
  runner::DynamicAMR&   _dynamicAMR;
  bool                  _terminatedGracefully = false;

  /**
   * Wall clock of every completed time step, the first one included. The throughput
   * measurement skips that step because its allocation cost is not representative, which
   * is the wrong basis for an extrapolation: the step still consumed wall clock and still
   * advanced the simulated time the estimate divides by.
   */
  double _timeSteppingSeconds = 0.0;

  /**
   * Drain wall clock already accumulated when the loop starts. Grid initialisation drains
   * through the same phase object and into the same measurement, and that work is one-off
   * setup, so the estimate must not extrapolate it as a recurring cost.
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
