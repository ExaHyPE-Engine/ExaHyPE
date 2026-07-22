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
  TimeLoop(PhaseContext& context, int timeStepStep, int plotStep, DynamicAMR& dynamicAMR);

  std::vector<std::string> widthSamples() const override;
  void                     run() override;

  /** True if the run ended early on a noncritical assertion (one final plot was written). */
  bool terminatedGracefully() const { return _terminatedGracefully; }

private:
  PhaseContext& _context;
  int           _timeStepStep;
  int           _plotStep;
  DynamicAMR&   _dynamicAMR;
  bool          _terminatedGracefully = false;

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
    double        minTimeStamp,
    double        maxTimeStamp,
    double        terminalTime,
    double        wallClockSeconds,
    unsigned long completedSteps,
    int           maxSteps
  );
  void        logPlot(int snapshot, int numberOfSnapshots, double timeStamp, double wallClockSeconds);
  std::string formatETA(double fraction) const;
};
