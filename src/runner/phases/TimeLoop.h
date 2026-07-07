// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Phase.h"

#include <string>
#include <vector>

namespace runner {
  class TimeLoop;
} // namespace runner

class runner::TimeLoop final : public runner::Phase {
public:
  TimeLoop(PhaseContext& context, int timeStepStep, int plotStep);

  std::vector<std::string> widthSamples() const override;
  void                     run() override;

  /** True if the run ended early on a noncritical assertion (one final plot was written). */
  bool terminatedGracefully() const { return _terminatedGracefully; }

private:
  PhaseContext& _context;
  int           _timeStepStep;
  int           _plotStep;
  bool          _terminatedGracefully = false;

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
